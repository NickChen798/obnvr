
#ifndef __OB_READER_H__
#define __OB_READER_H__

#define OBR_PLATE_LEN   16
typedef struct {

    int     ch;
    int     id;
    char    plate[OBR_PLATE_LEN];
    time_t  sec;
    time_t  usec;
    int     data_type;  // -1=N/A, 0=H.264, 1=JPEG
    void   *buffer;     // keep in handle struct, release at obr_free
    int     size;
    int     width;
    int     height;

} obr_result_t;

#define OBR_DT_NONE         -1
#define OBR_DT_H264         0
#define OBR_DT_JPEG         1

#define OBR_EV_PLATE        0
#define OBR_EV_VLOSS        1
#define OBR_EV_VRECOVERY    2
#define OBR_EV_VIDEO        3

#define OBR_IMG_NONE        -1
#define OBR_IMG_FULL        0
#define OBR_IMG_PLATE       1

#define OBR_ORDER_ASCENT    0
#define OBR_ORDER_DESCENT   1

extern void *obr_alloc(int vs1_ch, int id, time_t start, time_t end);
extern int   obr_setup(void *handle, int event_type, int image_type, int order);
extern int   obr_free(void *handle);
extern int   obr_read(void *handle, obr_result_t *result);
extern int   obr_snapshot(int ch, time_t ts, void *buf, int bsize, int x, int y, int w, int h);

#endif // __OB_READER_H__

