#include "inverted.c"

int main() {
    ts_docid_generator_t generator;
    ts_docid_generator_init(&generator);
    ts_document_t *document = ts_document_new(&generator);

    ts_inverted_t *inverted = ts_inverted_new("hello");

    ts_inverted_free(inverted);
    return 0;
}
