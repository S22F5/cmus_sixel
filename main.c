#define _GNU_SOURCE
#include <fcntl.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <sixel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// 2.4 times faster then shell version but not complete.

static inline int openCmusSocket() {
  const char *xdgRuntimeDir = getenv("XDG_RUNTIME_DIR");
  char CmusSocketPath[256];
  snprintf(CmusSocketPath, sizeof(CmusSocketPath), "%s/cmus-socket", xdgRuntimeDir);
  struct sockaddr_un addr;
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, CmusSocketPath, sizeof(addr.sun_path) - 1);
  int conn = connect(sock, (struct sockaddr *)&addr, sizeof(addr));

  return sock;
}

typedef struct {
  unsigned char *data;
  int size;
} ImageData;

static inline ImageData getMusicCover(const char *musicPath) {
  AVFormatContext *fmt_ctx = NULL;
  AVPacket *preferred_pkt = NULL;
  ImageData result = {NULL, 0};

  if (avformat_open_input(&fmt_ctx, musicPath, NULL, NULL) != 0) {
    exit(0);
  }

  if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
    exit(0);
  }

  // find image stream preferably with "front" comment
  for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
    AVStream *stream = fmt_ctx->streams[i];
    if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
      AVDictionaryEntry *tag = av_dict_get(stream->metadata, "comment", NULL, 0);
      if (tag && strcasestr(tag->value, "front")) {
        preferred_pkt = &stream->attached_pic; // front comment found
        break;
      } else {
        preferred_pkt = &stream->attached_pic; // front comment not found
      }
    }
  }

  if (preferred_pkt) {
    result.data = malloc(preferred_pkt->size);
    if (result.data) {
      memcpy(result.data, preferred_pkt->data, preferred_pkt->size);
      result.size = preferred_pkt->size;
    }
  } else {
    exit(0);
  }

  avformat_close_input(&fmt_ctx);
  return result;
}

static inline void drawSixel(int outfd, ImageData *img, int targetHeight, int palette, int cursX, int cursY) { // should use sixel canvas later so we dont have to use temp and would also fix the fragmented write
  sixel_encoder_t *encoder = NULL;
  char options[32], heightOpt[16], colorOpt[16];
  SIXELSTATUS status;

  FILE *fd;
  fd= fopen("/tmp/sixel_X", "wb");
  if (fd == NULL)
    exit(0);
  fwrite(img->data, img->size, 1, fd);
  fclose(fd);

  status = sixel_encoder_new(&encoder, NULL);

  // set encoder options
  snprintf(options, sizeof(options), "/dev/fd/%d", outfd);
  sixel_encoder_setopt(encoder, SIXEL_OPTFLAG_OUTPUT, options);

  snprintf(colorOpt, sizeof(colorOpt), "%d", palette);
  sixel_encoder_setopt(encoder, SIXEL_OPTFLAG_COLORS, colorOpt);

  snprintf(heightOpt, sizeof(heightOpt), "%d", targetHeight);
  sixel_encoder_setopt(encoder, SIXEL_OPTFLAG_HEIGHT, heightOpt);

  char buf[32];
  snprintf(buf, sizeof(buf), "\0337\033[%d;%dH", cursX, cursY);
  write(outfd, buf, strlen(buf)); // set cursor location
  sixel_encoder_encode(encoder, "/tmp/sixel_X");
  write(outfd, "\0338", sizeof("\0338") - 1); // restore cursor

  sixel_encoder_unref(encoder);
}

int main(int argc, char const *argv[]) {
  // check for change
  if (strcmp(argv[2], "playing") != 0) {
    exit(0);
  }

  // get embeded image out of music
  ImageData coverImage = getMusicCover(argv[4]);

  // cmus remote socket
  int cmusSock = openCmusSocket();
  write(cmusSock, "refresh\n", 9);

  // read config file
  FILE *configFp;
  int sixPalette = 64; // fallback values outside else for lsp
  int sixMult = 50;
  int sixOffsX = 3;
  int sixOffsY = 0;
  const char *xdgConfigDir = getenv("XDG_CONFIG_HOME");
  char cmusConfigPath[256];
  snprintf(cmusConfigPath, sizeof(cmusConfigPath), "%s/cmus/cmus_sixel.conf", xdgConfigDir);
  if ((configFp = fopen(cmusConfigPath, "r")) != NULL) {
    fscanf(configFp, "%*s %d\n", &sixPalette);
    fscanf(configFp, "%*s %d\n", &sixMult);
    fscanf(configFp, "%*s %d\n", &sixOffsX);
    fscanf(configFp, "%*s %d\n", &sixOffsY);
    fclose(configFp);
  }

  // get terminal size
  int ttyfd = open("/dev/tty", O_WRONLY);
  struct winsize terminalW;
  ioctl(ttyfd, TIOCGWINSZ, &terminalW);

  // get sixel size
  int sixSize = terminalW.ws_ypixel * sixMult / 100;
  if (terminalW.ws_row == 0 || terminalW.ws_col == 0) {
    terminalW.ws_row = 75;
    terminalW.ws_col = 310;
  }

  // get sixel position
  int cursX = terminalW.ws_row - (sixSize / (terminalW.ws_ypixel / terminalW.ws_row)) - sixOffsX;
  int cursY = terminalW.ws_col - (sixSize / (terminalW.ws_xpixel / terminalW.ws_col)) - sixOffsY;

  // draw sixel
  usleep(1500);
  drawSixel(ttyfd, &coverImage, sixSize, sixPalette, cursX, cursY);

  // cleanup
  free(coverImage.data);
  close(cmusSock);
  close(ttyfd);
  exit(0);
}
