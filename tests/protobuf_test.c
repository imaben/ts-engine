#include <stdio.h>
#include "protocol/protocol.pb-c.h"

#define FIELD_COUNT 4
#define TMP_FILE "/tmp/ts_protobuf_test_bin"

void encode() {
    Document document;
    Field **fields = calloc(FIELD_COUNT, sizeof(Field *));
    for (int i = 0; i < FIELD_COUNT; i++) {
        fields[i] = malloc(sizeof(Field));
        field__init(fields[i]);
        asprintf(&fields[i]->term, "term%d", i);
        fields[i]->type = 2;
        fields[i]->valueint32 = i;
    }
    document__init(&document);
    document.pk = 100;
    document.n_fields = FIELD_COUNT;
    document.fields = fields;

    size_t size = document__get_packed_size(&document);
    char *buf = malloc(size);
    document__pack(&document, buf);
    FILE *fp = fopen(TMP_FILE, "wb");
    fwrite(buf, size, 1, fp);
    fclose(fp);
    free(buf);
    printf("binary file saved to %s\n", TMP_FILE);
    for (int i = 0; i < FIELD_COUNT; i++) {
        free(fields[i]->term);
        free(fields[i]);
    }
    free(fields);

}

void decode() {
    uint8_t buf[1024] = {0};
    FILE *fp = fopen(TMP_FILE, "rb");
    if (fp == NULL) {
        fprintf(stderr, "please encode first\n");
        return;
    }
    size_t size = fread(buf, 1, sizeof(buf), fp);
    printf("read file size:%d\n", size);
    fclose(fp);
    Document *document = document__unpack(NULL, size, buf);
    if (document == NULL) {
        fprintf(stderr, "unserialize fail\n");
        return;
    }
    printf("===== document =====\n");
    printf("document.pk:%llu\n", document->pk);
    printf("document.n_fields:%llu\n", document->n_fields);
    printf("===== fields =====\n");
    for (int i = 0; i < document->n_fields; i++) {
        printf("field, term:%s, valueint32:%d\n", document->fields[i]->term,
                document->fields[i]->valueint32);
    }
    document__free_unpacked(document, NULL);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "./protobuf_test encode|decode\n");
        exit(1);
    }
    if (strcmp(argv[1], "encode") == 0) {
        encode();
    } else if (strcmp(argv[1], "decode") == 0) {
        decode();
    } else {
        fprintf(stderr, "./protobuf_test encode|decode\n");
    }
    return 0;
}

