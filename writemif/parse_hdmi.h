#ifndef __HDMI_PARSE_H__
#define __HDMI_PARSE_H__

int hdmi_parse(char *input_file, char *output_file);
int hdmi_encode(char *filename, char *rgbfile, char *outputfile);

extern int q_htotal;
extern int q_vtotal;
extern int q_hactive;
extern int q_vactive;
extern int q_colordepth;
extern int q_colorspace;

#endif
