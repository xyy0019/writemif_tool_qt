#ifndef _PKTINFO_H__
#define _PKTINFO_H__

enum pkt_type_e {
    PKT_TYPE_NULL = 0x0,
    PKT_TYPE_ACR = 0x1,
    /*PKT_TYPE_AUD_SAMPLE = 0x2,*/
    PKT_TYPE_GCP = 0x3,
    PKT_TYPE_ACP = 0x4,
    PKT_TYPE_ISRC1 = 0x5,
    PKT_TYPE_ISRC2 = 0x6,
    /*PKT_TYPE_1BIT_AUD = 0x7,*/
    /*PKT_TYPE_DST_AUD = 0x8,*/
    /*PKT_TYPE_HBIT_AUD = 0x9,*/
    PKT_TYPE_GAMUT_META = 0xa,
    /*PKT_TYPE_3DAUD = 0xb,*/
    /*PKT_TYPE_1BIT3D_AUD = 0xc,*/
    PKT_TYPE_AUD_META = 0xd,
    /*PKT_TYPE_MUL_AUD = 0xe,*/
    /*PKT_TYPE_1BITMUL_AUD = 0xf,*/

    PKT_TYPE_INFOFRAME_VSI = 0x81,
    PKT_TYPE_INFOFRAME_AVI = 0x82,
    PKT_TYPE_INFOFRAME_SPD = 0x83,
    PKT_TYPE_INFOFRAME_AUD = 0x84,
    PKT_TYPE_INFOFRAME_MPEGSRC = 0x85,
    PKT_TYPE_INFOFRAME_NVBI = 0x86,
    PKT_TYPE_INFOFRAME_DRM = 0x87,
    PKT_TYPE_EMP = 0x7f,

    PKT_TYPE_UNKNOWN,
};

/* AVI infoFrame packet - 0x82 */
struct avi_infoframe_st {
    unsigned char pkttype;
    unsigned char version;
    unsigned char length;
    /*PB0*/
    unsigned char checksum;
    union cont_u {
        struct v1_st {
            /*byte 1*/
            unsigned char scaninfo:2;		/* S1,S0 */
            unsigned char barinfo:2;		/* B1,B0 */
            unsigned char activeinfo:1;		/* A0 */
            unsigned char colorindicator:2;		/* Y1,Y0 */
            unsigned char rev0:1;
            /*byte 2*/
            unsigned char fmt_ration:4;		/* R3-R0 */
            unsigned char pic_ration:2;		/* M1-M0 */
            unsigned char colorimetry:2;		/* C1-C0 */
            /*byte 3*/
            unsigned char pic_scaling:2;		/* SC1-SC0 */
            unsigned char rev1:6;
            /*byte 4*/
            unsigned char rev2:8;
            /*byte 5*/
            unsigned char rev3:8;
        } v1;
        struct v4_st { /* v2=v3=v4 */
            /*byte 1*/
            unsigned char scaninfo:2;		/* S1,S0 */
            unsigned char barinfo:2;		/* B1,B0 */
            unsigned char activeinfo:1;		/* A0 1 */
            unsigned char colorindicator:3;		/* Y2-Y0 */
            /*byte 2*/
            unsigned char fmt_ration:4;		/* R3-R0 */
            unsigned char pic_ration:2;		/* M1-M0 */
            unsigned char colorimetry:2;		/* C1-C0 */
            /*byte 3*/
            unsigned char pic_scaling:2;		/* SC1-SC0 */
            unsigned char qt_range:2;		/* Q1-Q0 */
            unsigned char ext_color:3;		/* EC2-EC0 */
            unsigned char it_content:1;		/* ITC */
            /*byte 4*/
            unsigned char vic:8;			/* VIC7-VIC0 */
            /*byte 5*/
            unsigned char pix_repeat:4;		/* PR3-PR0 */
            unsigned char content_type:2;		/* CN1-CN0 */
            unsigned char ycc_range:2;		/* YQ1-YQ0 */
        } v4;
    } cont;
    /*byte 6,7*/
    unsigned int line_num_end_topbar:16;	/*littel endian can use*/
    /*byte 8,9*/
    unsigned int line_num_start_btmbar:16;
    /*byte 10,11*/
    unsigned int pix_num_left_bar:16;
    /*byte 12,13*/
    unsigned int pix_num_right_bar:16;
    /* byte 14 */
    unsigned char additional_colorimetry;
};

/* general control pkt - 0x3 */
struct gcp_pkt_st {
    /*packet header*/
    unsigned char pkttype;
    unsigned char hb1_zero;
    unsigned char hb2_zero;
    unsigned char rsvd;
    /*sub packet*/
    struct gcp_sbpkt_st {
        /*SB0*/
        unsigned char set_avmute:1;
        unsigned char sb0_zero0:3;
        unsigned char clr_avmute:1;
        unsigned char sb0_zero1:3;
        /*SB1*/
        unsigned char colordepth:4;
        unsigned char pixel_pkg_phase:4;
        /*SB2*/
        unsigned char def_phase:1;
        unsigned char sb2_zero:7;
        /*SB3*/
        unsigned char sb3_zero;
        /*SB4*/
        unsigned char sb4_zero;
        /*SB5*/
        unsigned char sb5_zero;
        /*SB6*/
        unsigned char sb6_zero;
    } sbpkt;
};

extern struct avi_infoframe_st avi_info;

extern struct gcp_pkt_st gcp_info;
void hdmi_parse_pkt_info(unsigned int *head, unsigned int *body_low, unsigned int *body_high);
void hdmi_gcp_info(unsigned int *head, unsigned int *body_low, unsigned int *body_high);

#endif
