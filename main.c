#include <linux/limits.h>
#define _GNU_SOURCE
#include <fcntl.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
#include <limits.h>
#include <sixel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

typedef struct {
  int sixPalette;
  int size;
  int offsX;
  int offsY;
  int useKitty;
  int forceSquare;
} Config;

static inline Config parseConfig(const char *configPath) {
  FILE *configFp = fopen(configPath, "r");
  if (configFp != NULL) {
    Config config = {64, 50, 3, 0, 0, 0};
    fscanf(configFp, "%*s %d\n", &config.sixPalette);
    fscanf(configFp, "%*s %d\n", &config.size);
    fscanf(configFp, "%*s %d\n", &config.offsX);
    fscanf(configFp, "%*s %d\n", &config.offsY);
    fscanf(configFp, "%*s %d\n", &config.useKitty);
    fscanf(configFp, "%*s %d\n", &config.forceSquare);
    fclose(configFp);
    return config;
  } else {
    exit(1);
  }
}

static inline int openCmusSocket() {
  const char *xdgRuntimeDir = getenv("XDG_RUNTIME_DIR");
  char cmusSocketPath[PATH_MAX];
  snprintf(cmusSocketPath, sizeof(cmusSocketPath), "%s/cmus-socket", xdgRuntimeDir);
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    exit(1);
  }
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, cmusSocketPath, sizeof(addr.sun_path) - 1);
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) exit(1);

  return sock;
}

typedef struct {
  unsigned char *data;
  int size;
  int width;
  int height;
} ImageData;

static inline ImageData getCover(const char *musicPath, int maxW, int maxH, int forceSquare) {
  AVFormatContext *fmtCtx = NULL;
  AVStream *preferredStream = NULL;
  AVPacket *preferredPkt = NULL;
  ImageData result = {NULL, 0, 0, 0};

  if (avformat_open_input(&fmtCtx, musicPath, NULL, NULL) != 0) {
    exit(1);
  }
  if (avformat_find_stream_info(fmtCtx, NULL) < 0) {
    avformat_close_input(&fmtCtx);
    exit(1);
  }

  // find image stream preferably with "front" comment
  for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
    AVStream *stream = fmtCtx->streams[i];
    if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
      AVDictionaryEntry *tag =
          av_dict_get(stream->metadata, "comment", NULL, 0);
      if (tag && strcasestr(tag->value, "front")) {
        preferredStream = stream;
        preferredPkt = &stream->attached_pic;
        break;
      } else if (!preferredPkt) {
        preferredStream = stream;
        preferredPkt = &stream->attached_pic;
      }
    }
  }
  if (!preferredPkt) {
    avformat_close_input(&fmtCtx);
    exit(1);
  }

  // setup decoder
  const AVCodec *decoder = avcodec_find_decoder(preferredStream->codecpar->codec_id);
  AVCodecContext *codecCtx = avcodec_alloc_context3(decoder);
  avcodec_parameters_to_context(codecCtx, preferredStream->codecpar);
  if (avcodec_open2(codecCtx, decoder, NULL) < 0) {
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    exit(1);
  }

  // send packet and get raw frame
  if (avcodec_send_packet(codecCtx, preferredPkt) < 0) {
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    exit(1);
  }
  AVFrame *frame = av_frame_alloc();
  if (avcodec_receive_frame(codecCtx, frame) < 0) {
    av_frame_free(&frame);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    exit(1);
  }

  // compute dst. size
  int dstW = maxW;
  int dstH = maxH;
  if (forceSquare) {
    int s = maxW < maxH ? maxW : maxH;
    dstW = dstH = s > 0 ? s : 1;
  } else {
    int srcW = frame->width > 0 ? frame->width : 1;
    int srcH = frame->height > 0 ? frame->height : 1;
    double scaleW = (double)maxW / (double)srcW;
    double scaleH = (double)maxH / (double)srcH;
    double scale = scaleW < scaleH ? scaleW : scaleH;
    if (scale <= 0.0) scale = 1.0;
    dstW = (int)(srcW * scale);
    dstH = (int)(srcH * scale);
    if (dstW < 1) dstW = 1;
    if (dstH < 1) dstH = 1;
  }

  // convert to RGB24 and resize
  struct SwsContext *swsCtx = sws_getContext(frame->width, frame->height, frame->format, dstW, dstH, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
  int rgbBufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, dstW, dstH, 1);
  unsigned char *rgbBuffer = malloc(rgbBufSize);
  AVFrame *rgbFrame = av_frame_alloc();
  av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, rgbBuffer, AV_PIX_FMT_RGB24, dstW, dstH, 1);
  sws_scale(swsCtx, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height, rgbFrame->data, rgbFrame->linesize);

  result.data = rgbBuffer;
  result.size = rgbBufSize;
  result.width = dstW;
  result.height = dstH;

  // cleanup
  av_frame_free(&rgbFrame);
  sws_freeContext(swsCtx);
  av_frame_free(&frame);
  avcodec_free_context(&codecCtx);
  avformat_close_input(&fmtCtx);

  return result;
}

static int writeSixelBuffer(char *data, int size, void *priv) {
  char **out = (char **)priv;
  size_t oldLen = *out ? strlen(*out) : 0;
  char *p = realloc(*out, oldLen + size + 1);
  if (!p) {
    exit(1);
  }
  *out = p;
  memcpy(*out + oldLen, data, size);
  (*out)[oldLen + size] = '\0';
  return size;
}

static inline void drawSixel(int outfd, ImageData *img, int cursX, int cursY, int palette) {
  int cmusSock = openCmusSocket();
  write(cmusSock, "refresh\n", 8);
  close(cmusSock);

  char *writeBuff = NULL;
  // cursor move (save + move)
  char cursBuff[64];
  size_t cursLen = snprintf(cursBuff, sizeof(cursBuff), "\0337\033[%d;%dH", cursX, cursY);
  writeSixelBuffer(cursBuff, cursLen, &writeBuff);
  // encode sixel using actual dimensions
  sixel_dither_t *dither;
  sixel_dither_new(&dither, palette, NULL);
  sixel_dither_initialize(dither, img->data, img->width, img->height, SIXEL_PIXELFORMAT_RGB888, LARGE_NORM, REP_CENTER_BOX, QUALITY_LOW);
  sixel_output_t *output;
  sixel_output_new(&output, writeSixelBuffer, &writeBuff, NULL);
  sixel_encode(img->data, img->width, img->height, SIXEL_PIXELFORMAT_RGB888, dither, output);
  // cursor restore
  writeSixelBuffer("\0338", 2, &writeBuff);
  // write buffer to outfd
  write(outfd, writeBuff, strlen(writeBuff));

  // cleanup
  free(writeBuff);
  sixel_dither_destroy(dither);
  sixel_output_destroy(output);
}

static inline void drawKitty(int outfd, ImageData *img, int cursX, int cursY) {
  // create shared memory object
  int shm = shm_open("/kittyCover", O_CREAT | O_RDWR, 0666);
  if (shm < 0)
    exit(1);
  if (ftruncate(shm, img->size) < 0) {
    close(shm);
    exit(1);
  }

  // map and copy image data
  void *ptr = mmap(NULL, img->size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
  if (ptr == MAP_FAILED) {
    close(shm);
    exit(1);
  }
  memcpy(ptr, img->data, img->size);
  munmap(ptr, img->size);
  close(shm);

  // build kitty image display buffer
  char buf[400];
  int len = snprintf(buf, sizeof(buf),
                     "\033_Ga=d\033\\"  // kitty delete all visible images
                     "\0337"            // save cursor position
                     "\033[%d;%dH"      // move cursor to cursX,cursY
                     "\033_Ga=T,"       // kitty action (transmit+display)
                     "f=24,"            // image format (rgb24)
                     "s=%d,v=%d,"       // image width,height
                     "t=s,m=0,"         // transmission medium, more data available
                     "S=%d;"            // image buffer size
                     "L2tpdHR5Q292ZXI=" // base64 <(printf "/kittyCover")
                     "\033\\"           // end kitty data
                     "\0338",           // restore saved cursor position
                     cursX, cursY, img->width, img->height, img->size);

  // write image display buffer
  write(outfd, buf, len);
}

int main(int argc, char *argv[]) {
  // check if cmus is playing
  if (strcmp(argv[2], "playing") != 0) {
    exit(0);
  }

  // read config file
  const char *xdgConfigDir = getenv("XDG_CONFIG_HOME");
  char cmusConfigPath[PATH_MAX];
  snprintf(cmusConfigPath, sizeof(cmusConfigPath), "%s/cmus/cmus_sixel.conf", xdgConfigDir);
  Config config = parseConfig(cmusConfigPath);

  int ttyfd = open("/dev/tty", O_WRONLY);

  // get terminal size
  struct winsize terminalW;
  ioctl(ttyfd, TIOCGWINSZ, &terminalW);

  const int FALLBACK_CELL_W = 8;
  const int FALLBACK_CELL_H = 16;
  if (terminalW.ws_xpixel == 0) terminalW.ws_xpixel = terminalW.ws_col * FALLBACK_CELL_W;
  if (terminalW.ws_ypixel == 0) terminalW.ws_ypixel = terminalW.ws_row * FALLBACK_CELL_H;
  if (terminalW.ws_row == 0) terminalW.ws_row = 24;
  if (terminalW.ws_col == 0) terminalW.ws_col = 80;

  int maxCoverH = (int)((long)terminalW.ws_ypixel * config.size / 100);
  int maxCoverW = (int)((long)terminalW.ws_xpixel * config.size / 100);
  if (maxCoverH < 1) maxCoverH = 1;
  if (maxCoverW < 1) maxCoverW = 1;

  // extract RGB24 cover
  ImageData coverImage = getCover(argv[4], maxCoverW, maxCoverH, config.forceSquare);

  // get terminal character size
  int pixelsPerCol = terminalW.ws_xpixel / terminalW.ws_col;
  int pixelsPerRow = terminalW.ws_ypixel / terminalW.ws_row;
  if (pixelsPerCol <= 0) pixelsPerCol = FALLBACK_CELL_W;
  if (pixelsPerRow <= 0) pixelsPerRow = FALLBACK_CELL_H;
  int coverCols = (coverImage.width  + pixelsPerCol - 1) / pixelsPerCol;
  int coverRows = (coverImage.height + pixelsPerRow - 1) / pixelsPerRow;

  // compute cursor position
  int cursX = terminalW.ws_row - coverRows - config.offsX + 1;
  int cursY = terminalW.ws_col - coverCols - config.offsY + 1;

  if (config.useKitty == 1) {
    drawKitty(ttyfd, &coverImage, cursX, cursY);
  } else {
    drawSixel(ttyfd, &coverImage, cursX, cursY, config.sixPalette);
  }

  //cleanup
  free(coverImage.data);
  close(ttyfd);
  exit(0);
}

