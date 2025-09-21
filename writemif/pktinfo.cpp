#include <stdio.h>
#include "pktinfo.h"
struct avi_infoframe_st avi_info;
struct gcp_pkt_st gcp_info;

void hdmi_avi_info(unsigned int *head, unsigned int *body_low, unsigned int *body_high)
{
   avi_info.pkttype = *head >> 24;
   avi_info.version = (*head >> 16) & 0xff;
   avi_info.length = (*head >> 8) & 0xff;
   avi_info.cont.v4.scaninfo = (*body_low >> 16) & 0x3;
   avi_info.cont.v4.barinfo = (*body_low >> 18) & 0x3;
   avi_info.cont.v4.activeinfo = (*body_low >> 20) & 0x1;
   avi_info.cont.v4.colorindicator = (*body_low >> 21) & 0x3;
   avi_info.cont.v4.fmt_ration = (*body_low >> 8) & 0xf;
   avi_info.cont.v4.pic_ration = (*body_low >> 12) & 0x3;
   avi_info.cont.v4.colorimetry = (*body_low >> 14) & 0x3;
   avi_info.cont.v4.pic_scaling = (*body_low) & 0x3;
   avi_info.cont.v4.qt_range = (*body_low >> 2) & 0x3;
   avi_info.cont.v4.ext_color = (*body_low >> 4) & 0x7;
   avi_info.cont.v4.ext_color = (*body_low >> 7) & 0x1;
   avi_info.cont.v4.vic = (*body_high >> 24);
   printf("*****AVI INFOFRAME*****\n");
   printf("pkttype = 0x%x\n", avi_info.pkttype);
   printf("version = 0x%x\n", avi_info.version);
   printf("length = 0x%x\n", avi_info.length);
   printf("scaninfo = 0x%x\n", avi_info.cont.v4.scaninfo);
   printf("barinfo = 0x%x\n", avi_info.cont.v4.barinfo);
   printf("activeinfo = 0x%x\n", avi_info.cont.v4.activeinfo);
   printf("colorindicator = 0x%x\n", avi_info.cont.v4.colorindicator);
   printf("fmt_ration = 0x%x\n", avi_info.cont.v4.fmt_ration);
   printf("pic_ration = 0x%x\n", avi_info.cont.v4.pic_ration);
   printf("colorimetry = 0x%x\n", avi_info.cont.v4.colorimetry);
   printf("pic_scaling = 0x%x\n", avi_info.cont.v4.pic_scaling);
   printf("hw_vic = %d\n", avi_info.cont.v4.vic);
}

void hdmi_gcp_info(unsigned int *head, unsigned int *body_low, unsigned int *body_high)
{
   gcp_info.pkttype = *head >> 24;
   gcp_info.hb1_zero = *head >> 16;
   gcp_info.hb2_zero = *head >> 8;
   gcp_info.sbpkt.set_avmute = (*body_low >> 24) & 0x1;
   gcp_info.sbpkt.clr_avmute = (*body_low >> 28) & 0x1;
   gcp_info.sbpkt.colordepth = (*body_low >> 16) & 0xf;
   gcp_info.sbpkt.pixel_pkg_phase = (*body_low >> 20) & 0xf;
   gcp_info.sbpkt.def_phase = (*body_low >> 8) & 0x1;
   printf("*****CGP INFO*****\n");
   printf("pkttype = 0x%x\n", gcp_info.pkttype);
   printf("set_avmute = 0x%x\n", gcp_info.sbpkt.set_avmute);
   printf("clr_avmute = 0x%x\n", gcp_info.sbpkt.clr_avmute);
   printf("colordepth = 0x%x\n", gcp_info.sbpkt.colordepth);
   printf("pixel_pkg_phase = 0x%x\n", gcp_info.sbpkt.pixel_pkg_phase);
   printf("def_phase = 0x%x\n", gcp_info.sbpkt.def_phase);
}

void hdmi_parse_pkt_info(unsigned int *head, unsigned int *body_low, unsigned int *body_high)
{
   unsigned char pkttype = *head >> 24;
   switch (pkttype) {
   /*infoframe pkt*/
   case PKT_TYPE_INFOFRAME_AVI:
       hdmi_avi_info(head, body_low, body_high);
       break;
   case PKT_TYPE_INFOFRAME_SPD:
       break;
   case PKT_TYPE_INFOFRAME_AUD:
       break;
   case PKT_TYPE_INFOFRAME_MPEGSRC:
       break;
   case PKT_TYPE_INFOFRAME_NVBI:
       break;
   case PKT_TYPE_INFOFRAME_DRM:
       break;
   /*other pkt*/
   case PKT_TYPE_ACR:
       break;
   case PKT_TYPE_GCP:
       hdmi_gcp_info(head, body_low, body_high);
       break;
   case PKT_TYPE_ACP:
       break;
   case PKT_TYPE_ISRC1:
       break;
   case PKT_TYPE_ISRC2:
       break;
   case PKT_TYPE_GAMUT_META:
       break;
   case PKT_TYPE_AUD_META:
       break;
   case PKT_TYPE_EMP:
       break;
   default:
       /*printf("warning: not support\n");
       printf("vsi->0x81:Vendor-Specific infoframe\n");
       printf("avi->0x82:Auxiliary video infoframe\n");
       printf("spd->0x83:Source Product Description infoframe\n");
       printf("aud->0x84:Audio infoframe\n");
       printf("mpeg->0x85:MPEG infoframe\n");
       printf("nvbi->0x86:NTSC VBI infoframe\n");
       printf("drm->0x87:DRM infoframe\n");
       printf("acr->0x01:audio clk regeneration\n");
       printf("gcp->0x03\n");
       printf("acp->0x04\n");
       printf("isrc1->0x05\n");
       printf("isrc2->0x06\n");
       printf("gmd->0x0a\n");
       printf("amp->0x0d\n");
       printf("emp->0x7f:EMP\n");*/
       break;
   }
}
