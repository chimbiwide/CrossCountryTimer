#include <vlm.h>
#include <mtmd-helper.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Context cells one strip may use: its image (under 400 for a strip of
// MAX_PIXELS), the prompt and the text read from it
#define STRIP_CONTEXT 1024
// More than the image tokens of the biggest strip
#define BATCH_SIZE 512

// GLM-OCR's own chat format with its plain-text task. The model writes the
// lines as printed ("12. LaMay, Ethan 1092 M 8"); roster_add_text() reads them.
#define PROMPT_START "[gMASK]<sop><|user|>\n"
#define PROMPT_END "Text Recognition:<|assistant|>"

// One strip being read: its text so far, and the token to feed next
typedef struct {
    char *text;
    int size;
    int length;
    int line_start;
    int last_row;
    llama_pos past;     // position of the next token; an image takes fewer positions than cells
    int cells;          // context cells used: the image, the prompt and the text
    llama_token token;
    bool reading;
    bool failed;
} Reader;

// One thread per core: half the logical CPUs on machines with hyperthreading.
// At most 8: on a desktop with 8 fast and 8 slow cores, 12 threads took 87 s
// where 8 took 66 s, the extra ones waiting on the slow cores.
int vlm_threads(void) {
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    int cpus = (int)info.dwNumberOfProcessors;
#else
    int cpus = (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
    int threads = cpus / 2;
    if (threads > 8) threads = 8;
    if (threads < 1) threads = 1;
    return threads;
}

static void log_errors(enum ggml_log_level level, const char *text, void *user_data) {
    (void)user_data;
    if (level == GGML_LOG_LEVEL_ERROR) fputs(text, stderr);
}

bool vlm_load(Vlm *vlm, const char *model_path, const char *mmproj_path, int threads) {
    memset(vlm, 0, sizeof(*vlm));
    llama_log_set(log_errors, NULL);
    mtmd_helper_log_set(log_errors, NULL);
    llama_backend_init();

    // CPU only: no layers on a GPU, and the vision encoder stays on the CPU too
    struct llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0;
    vlm->model = llama_model_load_from_file(model_path, model_params);
    if (vlm->model == NULL) return false;

    // each strip of a group is its own sequence, with STRIP_CONTEXT tokens of its own
    struct llama_context_params context_params = llama_context_default_params();
    context_params.n_ctx = STRIP_CONTEXT * READ_GROUP;
    context_params.n_seq_max = READ_GROUP;
    context_params.kv_unified = false;
    context_params.n_batch = BATCH_SIZE;
    context_params.n_ubatch = BATCH_SIZE;
    context_params.n_threads = threads;
    context_params.n_threads_batch = threads;
    vlm->context = llama_init_from_model(vlm->model, context_params);
    if (vlm->context == NULL) return false;

    struct mtmd_context_params vision_params = mtmd_context_params_default();
    vision_params.use_gpu = false;
    vision_params.print_timings = false;
    vision_params.warmup = false;
    vision_params.n_threads = threads;
    vlm->vision = mtmd_init_from_file(mmproj_path, vlm->model, vision_params);
    if (vlm->vision == NULL) return false;

    vlm->vocab = llama_model_get_vocab(vlm->model);
    vlm->vocab_size = llama_vocab_n_tokens(vlm->vocab);
    vlm->batch = llama_batch_init(READ_GROUP, 0, 1);
    snprintf(vlm->prompt, sizeof(vlm->prompt), "%s%s%s", PROMPT_START, mtmd_default_marker(), PROMPT_END);
    return true;
}

// Greedy: always the model's favourite token, so the same strips always give
// the same text. lead is how far it beat the runner-up, in logits. index is
// the token's place in the last batch, -1 for its last token.
static llama_token pick_token(Vlm *vlm, int index, float *lead) {
    const float *logits = llama_get_logits_ith(vlm->context, index);
    int best = 0;
    int second = 1;
    if (logits[second] > logits[best]) {
        best = 1;
        second = 0;
    }
    for (int i = 2; i < vlm->vocab_size; i++) {
        if (logits[i] > logits[best]) {
            second = best;
            best = i;
        }
        else if (logits[i] > logits[second]) second = i;
    }
    *lead = logits[best] - logits[second];
    return best;
}

// Whether the line at out + start, length bytes long, is already among the
// complete lines before it.
static bool repeated_line(const char *out, int start, int length) {
    int seen = 0;
    while (seen < start) {
        int seen_length = (int)(strchr(out + seen, '\n') - out) - seen;
        if (seen_length == length && memcmp(out + seen, out + start, length) == 0) return true;
        seen += seen_length + 1;
    }
    return false;
}

// Adds the model's next token to a strip's text, or ends the strip.
static void take_token(Vlm *vlm, Reader *reader, llama_token token) {
    if (llama_vocab_is_eog(vlm->vocab, token)) {
        reader->reading = false;
        return;
    }
    char piece[256];
    int n = llama_token_to_piece(vlm->vocab, token, piece, sizeof(piece), 0, false);
    if (n < 0 || reader->length + n >= reader->size) {
        reader->reading = false;
        reader->failed = true;
        return;
    }
    char *out = reader->text;
    memcpy(out + reader->length, piece, n);
    reader->length += n;
    out[reader->length] = '\0';
    reader->token = token;

    // Row numbers only go up, starting over at 1 under each school line, and
    // no line comes twice (every row has its own bib). When either breaks,
    // the model has started reading the strip again, so the strip is done.
    char *newline;
    while ((newline = strchr(out + reader->line_start, '\n')) != NULL) {
        int line_start = reader->line_start;
        int line_length = (int)(newline - out) - line_start;
        if (line_length > 0 && repeated_line(out, line_start, line_length)) {
            out[line_start] = '\0';
            reader->length = line_start;
            reader->reading = false;
            return;
        }
        int end = line_start + line_length;
        while (end > line_start && (out[end - 1] == ' ' || out[end - 1] == '\r')) end--;
        int row = 0;
        char dot = '\0';
        if (end > line_start && out[end - 1] == ')') reader->last_row = 0;
        else if (sscanf(out + line_start, "%d%c", &row, &dot) == 2 && dot == '.') {
            if (row <= reader->last_row) {
                out[line_start] = '\0';
                reader->length = line_start;
                reader->reading = false;
                return;
            }
            reader->last_row = row;
        }
        reader->line_start = (int)(newline - out) + 1;
    }
}

// The first strip that is not read to its end, count when all are.
static int first_unread(const Reader *readers, int count) {
    for (int i = 0; i < count; i++) {
        if (readers[i].reading || readers[i].failed) return i;
    }
    return count;
}

// Transcribes up to READ_GROUP strips into texts, size bytes each. Each
// strip's image goes in on its own; then every step adds one token to each
// strip still being read, so the model's weights come from memory once for
// all of them. Returns count when every strip was read, otherwise the index
// of the first strip that could not be.
int vlm_read(Vlm *vlm, const Crop *crops, int count, char *texts[], int size) {
    Reader readers[READ_GROUP];
    if (count > READ_GROUP) count = READ_GROUP;
    llama_memory_clear(llama_get_memory(vlm->context), true);

    for (int i = 0; i < count; i++) {
        Reader *reader = &readers[i];
        memset(reader, 0, sizeof(*reader));
        reader->text = texts[i];
        reader->size = size;
        reader->text[0] = '\0';
        vlm->lowest_lead[i] = INFINITY;

        mtmd_bitmap *bitmap = mtmd_bitmap_init(crops[i].width, crops[i].height, crops[i].rgb);
        mtmd_input_chunks *chunks = mtmd_input_chunks_init();
        mtmd_input_text text = {vlm->prompt, strlen(vlm->prompt), true, true};
        const mtmd_bitmap *bitmaps[] = {bitmap};
        int result = mtmd_tokenize(vlm->vision, chunks, &text, bitmaps, 1);
        if (result == 0) {
            reader->cells = (int)mtmd_helper_get_n_tokens(chunks);
            result = mtmd_helper_eval_chunks(vlm->vision, vlm->context, chunks, 0, i, BATCH_SIZE, true,
                                             &reader->past);
        }
        mtmd_input_chunks_free(chunks);
        mtmd_bitmap_free(bitmap);
        if (result != 0) {
            reader->failed = true;
            return first_unread(readers, i + 1);
        }

        // the logits are this strip's until the next strip's image goes in
        float lead = 0.0f;
        llama_token token = pick_token(vlm, -1, &lead);
        vlm->lowest_lead[i] = lead;
        reader->reading = true;
        take_token(vlm, reader, token);
    }

    while (true) {
        if (vlm->stop != NULL && atomic_load(vlm->stop)) return first_unread(readers, count);
        for (int i = 0; i < count; i++) {
            // the text didn't end within the strip's share of the context
            if (readers[i].reading && readers[i].cells >= STRIP_CONTEXT) readers[i].failed = true;
            if (readers[i].failed) return first_unread(readers, count);
        }
        int slot[READ_GROUP];
        vlm->batch.n_tokens = 0;
        for (int i = 0; i < count; i++) {
            Reader *reader = &readers[i];
            if (!reader->reading) continue;
            int k = vlm->batch.n_tokens++;
            vlm->batch.token[k] = reader->token;
            vlm->batch.pos[k] = reader->past++;
            vlm->batch.n_seq_id[k] = 1;
            vlm->batch.seq_id[k][0] = i;
            vlm->batch.logits[k] = true;
            reader->cells++;
            slot[i] = k;
        }
        if (vlm->batch.n_tokens == 0) return count;
        if (llama_decode(vlm->context, vlm->batch) != 0) return first_unread(readers, count);
        for (int i = 0; i < count; i++) {
            if (!readers[i].reading) continue;
            float lead = 0.0f;
            llama_token token = pick_token(vlm, slot[i], &lead);
            if (lead < vlm->lowest_lead[i]) vlm->lowest_lead[i] = lead;
            take_token(vlm, &readers[i], token);
        }
    }
}

void vlm_free(Vlm *vlm) {
    if (vlm->batch.token != NULL) llama_batch_free(vlm->batch);
    if (vlm->vision != NULL) mtmd_free(vlm->vision);
    if (vlm->context != NULL) llama_free(vlm->context);
    if (vlm->model != NULL) llama_model_free(vlm->model);
    memset(vlm, 0, sizeof(*vlm));
}
