#include "acutest.h"
#include "csd.h"
#include <stdio.h>

const char *csd_game_source = ""
                              "tetris {\n"
                              "  window {\n"
                              "    width: 1920,\n"
                              "    height: 1080,\n"
                              "    title: 'Tetris game',\n"
                              "    fullscreen: false\n"
                              "  }\n"
                              "  controls {\n"
                              "    left: 'a',\n"
                              "    right: 'd',\n"
                              "    confirm: 'e',\n"
                              "    pause: 'p'\n"
                              "  }\n"
                              "}";

const char *csd_dialog_source = ""
                                "{\n"
                                "en_US {\n"
                                "	title: 'Game over',\n"
                                "	play_again_prompt: 'Play again ?',\n"
                                "	play_again_accept: 'Yes',\n"
                                "	play_again_refuse: 'No'\n"
                                "},\n"
                                "fr_FR {\n"
                                "	title: 'Partie terminée',\n"
                                "	play_again_prompt: 'Souhaitez-vous rejouer ?',\n"
                                "	play_again_accept: 'Oui',\n"
                                "	play_again_refuse: 'Non'\n"
                                "},\n"
                                "es_ES {\n"
                                "	title: 'Juego terminado',\n"
                                "	play_again_prompt: 'Juega de nuevo ?',\n"
                                "	play_again_accept: 'Sí',\n"
                                "	play_again_refuse: 'No'\n"
                                "},\n"
                                "zh_CN {\n"
                                "	title: '游戏结束',\n"
                                "	play_again_prompt: '再玩一次 ？',\n"
                                "	play_again_accept: '是的',\n"
                                "	play_again_refuse: '不'\n"
                                "}\n"
                                "}";

const char *csd_features_source =
    ""
    "# You can add hash-delimited comments #\n"
    "{\n"
    "\n"
    "colors {\n"
    "	gray:  [128,128,128],\n"
    "	blue: '#0000FF',\n"
    "	yellow: 16776960\n"
    "},\n"
    "\n"
    "# utf-8 is supported #\n"
    "vietnamese: 'Lorem Ipsum chỉ đơn giản là một đoạn văn bản giả, được dùng vào việc \n"
    "trình bày và dàn trang phục vụ cho in ấn.\n"
    " Lorem Ipsum đã được sử dụng như một văn bản chuẩn cho ngành công nghiệp in ấn từ \n"
    "những năm 1500, khi một họa sĩ vô\n"
    " danh ghép nhiều đoạn văn bản với nhau để tạo thành một bản mẫu văn bản. Đoạn văn \n"
    "bản này không những đã tồn tại năm thế\n"
    " kỉ, mà khi được áp dụng vào tin học văn phòng, nội dung của nó vẫn không hề bị \n"
    "thay đổi. Nó đã được phổ biến trong\n"
    " những năm 1960 nhờ việc bán những bản giấy Letraset in những đoạn Lorem Ipsum, và "
    "\n"
    "gần đây hơn, được sử dụng trong các\n"
    " ứng dụng dàn trang.'\n"
    "}";

typedef struct csd_neq_status
{
    bool has_neq;
    char why[2048];
    csd_node *a;
    csd_node *b;
} csd_neq_status;

void csd_node_neq_cb(csd_node *a, csd_node *b, void *p)
{
    csd_neq_status *status = p;
    char sa[1024];
    char sb[1024];

    csd_write_string(sa, sizeof(sa), a, csd_format_standard);
    csd_write_string(sb, sizeof(sb), b, csd_format_standard);
    sprintf(status->why, "expected: %s\n", sa);
    sprintf(status->why, "%sgot: %s\n", status->why, sb);
    status->has_neq = true;
}

void csd_test_parse(const char *source, csd_document *expected)
{
    csd_document got = csd_parse(strdup(source));
    TEST_CHECK_(!got.error, "%s", got.reason);

    csd_neq_status status = {0};
    csd_eq_x(expected->head, got.head, &csd_node_neq_cb, &status);
    TEST_CHECK_(!status.has_neq, "%s", status.why);
}

void csd_test_parse_game(void)
{
    csd_document doc = {0};
    doc.head = csd_make_sequence(
        &doc, "tetris",
        csd_make_sequence(&doc, "window", csd_new_int(&doc, "width", 1920),
                          csd_new_int(&doc, "height", 1080),
                          csd_new_string(&doc, "title", "Tetris game"),
                          csd_new_boolean(&doc, "fullscreen", false)),
        csd_make_sequence(&doc, "controls", csd_new_string(&doc, "left", "a"),
                          csd_new_string(&doc, "right", "d"),
                          csd_new_string(&doc, "confirm", "e"),
                          csd_new_string(&doc, "pause", "p")));

    csd_test_parse(csd_game_source, &doc);
}

TEST_LIST = {
    {"parse game.sd", &csd_test_parse_game},
    {NULL, NULL},
};
