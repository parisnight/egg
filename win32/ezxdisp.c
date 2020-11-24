/*
 * win32/ezxdisp.c
 * This file is part of the ezxdisp library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>
#include <windows.h>
#include <windowsx.h>
#include <process.h>

#include "ezxdisp.h"

struct ezx_s {
  int size_x, size_y;
  ezx_color_t bgcolor;
  int closed;
  int quit;

  HWND hWnd;
  char *window_name;
  HBITMAP hBitmap;
  HDC hdcMem;
  HFONT hFont;
  CRITICAL_SECTION cSect;

  HANDLE hThread;
  HANDLE hWindowCreation;

  int                 cur_layer;
  struct ezx_figures *fig_head[EZX_NLAYER];
  struct ezx_figures *fig_tail[EZX_NLAYER];

  // 3d stuffs
  double scrdist, mag;
  double eye_x, eye_y, eye_z, eye_r;
  double cmat[3][3];
  double light_x, light_y, light_z;

  struct event_queue_s *event_queue;
};

typedef struct ezx_figures {
  enum {
    FPOINT2D,
    FLINE2D,
    FLINES2D,
    FLINE3D,
    FPOLY2D,
    FPOLY3D,
    FSTR2D,
    FSTR3D,
    FRECT2D,
    FFILLEDRECT2D,
    FCIRCLE2D,
    FFILLEDCIRCLE2D,
    FFILLEDCIRCLE3D,
    FARC2D,
    FFILLEDARC2D,
  } type;

  int x0, y0, z0, x1, y1, z1, r;
  double dx0, dy0, dz0, dx1, dy1, dz1, dr;
  double angle1, angle2;
  char  *str;

  int npoints;
  ezx_point2d_t *points_2d;
  ezx_point3d_t *points_3d;

  ezx_color_t         col;
  int                 width;
  struct ezx_figures *next;
} ezx_figures;

typedef struct ezx_pfigures {
  double               z;
  struct ezx_figures  *fig;
  struct ezx_pfigures *next;
} ezx_pfigures;

#define QID_BUTTON_PRESS 0
#define QID_OTHERS       1
typedef struct event_queue_s {
  int queue_num;
  CRITICAL_SECTION cSect;
  HANDLE *hEvent;
  struct queue_s {
    int size;
    struct event_queue_elem_s {
      unsigned int time;
      ezx_event_t *event;
      struct event_queue_elem_s *next;
    } *head, *tail;
  } *queue;
} event_queue_t;

const ezx_color_t ezx_black  = {0, 0, 0};
const ezx_color_t ezx_white  = {1, 1, 1};
const ezx_color_t ezx_grey25 = {0.25, 0.25, 0.25};
const ezx_color_t ezx_grey50 = {0.5, 0.5, 0.5};
const ezx_color_t ezx_grey75 = {0.75, 0.75, 0.75};
const ezx_color_t ezx_blue   = {0, 0, 1};
const ezx_color_t ezx_red    = {1, 0, 0};
const ezx_color_t ezx_green  = {0, 1, 0};
const ezx_color_t ezx_yellow = {1, 1, 0};
const ezx_color_t ezx_purple = {1, 0, 1};
const ezx_color_t ezx_pink   = {1, 0.5, 0.5};
const ezx_color_t ezx_cyan   = {0.5, 0.5, 1};
const ezx_color_t ezx_brown  = {0.5, 0, 0};
const ezx_color_t ezx_orange = {1, 0.5, 0};

static const char * const class_name = "ezx_window";

static void error_exit(const char *fmt, ...)
{
  va_list params;

  fprintf(stderr, "ezxdisp: ");
  va_start(params, fmt);
  vfprintf(stderr, fmt, params);
  va_end(params);
  fprintf(stderr, "\n");
  fflush(stderr);
  exit(EXIT_FAILURE);
}

static void sys_error_exit(const char *fmt, ...)
{
  int errno_save = errno;
  va_list params;

  fprintf(stderr, "ezxdisp: ");
  va_start(params, fmt);
  vfprintf(stderr, fmt, params);
  va_end(params);
  fprintf(stderr, ": %s\n", strerror(errno_save));
  fflush(stderr);
  exit(EXIT_FAILURE);
}

static inline void *xmalloc(size_t n)
{
  void *p;

  p = malloc(n);
  if (!p) sys_error_exit("malloc failed");
  
  return p;
}

static inline void *xcalloc(size_t n, size_t s)
{
  void *p;

  p = calloc(n, s);
  if (!p) sys_error_exit("calloc failed");

  return p;
}

static void win32_error_exit(const char *str)
{
  LPVOID lpBuffer;
  char *c;

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(),
		MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			 SORT_DEFAULT), (LPTSTR)&lpBuffer, 0, NULL);
  if ((c = strchr(lpBuffer, '\n')) != NULL) *c = '\0';
  error_exit("%s: %s", str, lpBuffer);
}

static event_queue_t *event_queue_new()
{
  int i;
  event_queue_t *q;

  q = xmalloc(sizeof(event_queue_t));
  q->queue_num = 2; /* one is for EZX_BUTTON_PRESS, and the other is for
		       other events */
  InitializeCriticalSection(&q->cSect);
  q->queue = xmalloc(sizeof(struct queue_s) * q->queue_num);
  q->hEvent = xmalloc(sizeof(HANDLE) * q->queue_num);
  for (i = 0; i < q->queue_num; i++) {
    q->hEvent[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
    q->queue[i].size = 0;
    q->queue[i].head = q->queue[i].tail = NULL;
  }

  return q;
}

static void event_queue_offer(event_queue_t *q, int id, ezx_event_t *event)
{
  struct queue_s *queue = &(q->queue[id]);
  struct event_queue_elem_s *new = xmalloc(sizeof(struct event_queue_elem_s));

  new->time = GetMessageTime();
  new->event = event;
  new->next = NULL;

  EnterCriticalSection(&q->cSect);

  if (queue->size == 0) queue->head = queue->tail = new;
  else queue->tail = queue->tail->next = new;
  (queue->size)++;
  if (!SetEvent(q->hEvent[id]))
    win32_error_exit("event_queue_offer: SetEvent failed");

  LeaveCriticalSection(&q->cSect);
}

static ezx_event_t *event_queue_remove(event_queue_t *q, int id)
{
  struct queue_s *queue = &(q->queue[id]);
  struct event_queue_elem_s *elem;
  ezx_event_t *event;

  if (WaitForSingleObject(q->hEvent[id], INFINITE) != WAIT_OBJECT_0)
    win32_error_exit("event_queue_remove: WaitForSingleObject failed");
  
  EnterCriticalSection(&q->cSect);

  elem = queue->head;
  assert(queue->size > 0);
  if (queue->size == 1) queue->head = queue->tail = NULL;
  else queue->head = elem->next;
  (queue->size)--;
  if (queue->size == 0) {
    if (!ResetEvent(q->hEvent[id]))
      win32_error_exit("event_queue_remove: ResetEvent failed");
  }

  LeaveCriticalSection(&q->cSect);
  
  event = elem->event;
  free(elem);
  return event;
}

static ezx_event_t *event_queue_remove_oldest(event_queue_t *q)
{
  struct queue_s *queue;
  struct event_queue_elem_s *elem;
  ezx_event_t *event;
  int i, oldest_id;
  DWORD ret;
  unsigned int min_time;

  ret = WaitForMultipleObjects(q->queue_num, q->hEvent, FALSE, INFINITE);
  if (ret < WAIT_OBJECT_0 || WAIT_OBJECT_0+q->queue_num <= ret)
    win32_error_exit("event_queue_remove_oldest: WaitForMultipleObjects failed");

  EnterCriticalSection(&q->cSect);

  oldest_id = ret - WAIT_OBJECT_0;
  assert(q->queue[oldest_id].size > 0);
  min_time = q->queue[oldest_id].head->time;
  for (i = oldest_id+1; i < q->queue_num; i++) {
    if (q->queue[i].head != NULL && q->queue[i].head->time < min_time) {
      oldest_id = i;
      min_time = q->queue[i].head->time;
    }
  }

  queue = &(q->queue[oldest_id]);
  elem = queue->head;
  if (queue->size == 1) queue->head = queue->tail = NULL;
  else queue->head = elem->next;
  (queue->size)--;
  if (queue->size == 0) {
    if (!ResetEvent(q->hEvent[oldest_id]))
      win32_error_exit("event_queue_remove_oldest: ResetEvent failed");
  }

  LeaveCriticalSection(&q->cSect);

  event = elem->event;
  free(elem);
  return event;
}

static void event_queue_free(event_queue_t *q)
{
  int i;
  struct event_queue_elem_s *elem, *next;

  for (i = 0; i < q->queue_num; i++) {
    next = q->queue[i].head;
    while (next) {
      elem = next;
      next = next->next;
      free(elem->event);
      free(elem);
    }
    CloseHandle(q->hEvent[i]);
  }
  free(q->queue);
  free(q->hEvent);
  DeleteCriticalSection(&q->cSect);
  free(q);
}

void ezx_wipe(ezx_t *e)
{
  int i;
  ezx_figures *f, *nf;

  for (i = 0; i < EZX_NLAYER; i++) {
    for (f = e->fig_head[i]; f != NULL; f = nf) {
      nf = f->next;
      if (f->type == FSTR3D) free(f->str);
      if (f->type == FSTR2D) free(f->str);
      free(f);
    }

    e->fig_head[i] = NULL;
    e->fig_tail[i] = NULL;
  }
}

void ezx_wipe_layer(ezx_t *e, int lay)
{
  ezx_figures *f, *nf;

  if (lay < 0 || EZX_NLAYER <= lay)
    error_exit("ezx_wipe_layer: invalid layer number %d", lay);
  
  for (f = e->fig_head[lay]; f != NULL; f = nf) {
    nf = f->next;
    if (f->type == FSTR3D) free(f->str);
    if (f->type == FSTR2D) free(f->str);
    free(f);
  }

  e->fig_head[lay] = NULL;
  e->fig_tail[lay] = NULL;
}

void ezx_select_layer(ezx_t *e, int lay)
{
  if (lay < 0 || EZX_NLAYER <= lay)
    error_exit("ezx_select_layer: invalid layer number %d", lay);

  e->cur_layer = lay;
}

void ezx_set_light_3d(ezx_t *e, double ex, double ey, double ez)
{
  double s = sqrt(ex * ex + ey * ey + ez * ez);

  if (s != 0) {
    e->light_x = ex / s;
    e->light_y = ey / s;
    e->light_z = ez / s;
  }
}

void ezx_set_view_3d(ezx_t *e, double ex, double ey, double ez, double vx,
		     double vy, double vz, double m)
{
  double x = vx - ex, y = vy - ey, z = vz - ez;
  double theta = atan2(y, x);
  double tmp = x * x + y * y;
  double phi;
  double st, ct, sp, cp;

  e->mag = m;

  e->eye_x = ex;
  e->eye_y = ey;
  e->eye_z = ez;
  e->eye_r = sqrt(x * x + y * y + z * z);

  if (tmp == 0) phi = M_PI / 2;
  else phi = acos(tmp / (sqrt(tmp) * e->eye_r));

  if (z < 0) phi = -phi;

  st = sin(theta);
  ct = cos(theta);
  sp = sin(phi);
  cp = cos(phi);

  e->cmat[0][0] = -st;
  e->cmat[0][1] = -ct * sp;
  e->cmat[0][2] = ct * cp;
  e->cmat[1][0] = ct;
  e->cmat[1][1] = -st * sp;
  e->cmat[1][2] = st * cp;
  e->cmat[2][0] = 0;
  e->cmat[2][1] = cp;
  e->cmat[2][2] = sp;
}

static double getz(ezx_t *e, double sx, double sy, double sz)
{
  sx -= e->eye_x;
  sy -= e->eye_y;
  sz -= e->eye_z;

  return sx * e->cmat[0][2] + sy * e->cmat[1][2] + sz * e->cmat[2][2];
}

void ezx_c3d_to_2d(ezx_t *e, double sx, double sy, double sz, double *dx,
		   double *dy)
{
  double x2, y2, z2, rz;

  sx -= e->eye_x;
  sy -= e->eye_y;
  sz -= e->eye_z;

  x2 = sx * e->cmat[0][0] + sy * e->cmat[1][0] + sz * e->cmat[2][0];
  y2 = sx * e->cmat[0][1] + sy * e->cmat[1][1] + sz * e->cmat[2][1];
  z2 = sx * e->cmat[0][2] + sy * e->cmat[1][2] + sz * e->cmat[2][2];

  rz = e->scrdist - z2;
  *dx = e->mag * e->scrdist * x2 / rz;
  *dy = e->mag * e->scrdist * y2 / rz;
}

static void clip_line(int *x0, int *y0, int *x1, int *y1, int width, int height)
{
  if (*x0 > *x1) {
    int t;
    t = *x0;
    *x0 = *x1;
    *x1 = t;
    t = *y0;
    *y0 = *y1;
    *y1 = t;
  }

  if (*x0 < 0 && *x1 >= 0) {
    *y0 = *y1 + (*y0 - *y1) * (0 - *x1) / (*x0 - *x1);
    *x0 = 0;
  }
  if (*x1 >= width && *x0 < width) {
    *y1 = *y0 + (*y1 - *y0) * (width - *x0) / (*x1 - *x0);
    *x1 = width;
  }

  if (*y0 > *y1) {
    int t;
    t = *x0;
    *x0 = *x1;
    *x1 = t;
    t = *y0;
    *y0 = *y1;
    *y1 = t;
  }

  if (*y0 < 0 && *y1 >= 0) {
    *x0 = *x1 + (*x0 - *x1) * (0 - *y1) / (*y0 - *y1);
    *y0 = 0;
  }
  if (*y1 >= height && *y0 < height) {
    *x1 = *x0 + (*x1 - *x0) * (height - *y0) / (*y1 - *y0);
    *y1 = height;
  }
}

static void figure_list_add_tail(ezx_t *e, ezx_figures *nf)
{
  int lay = e->cur_layer;
  
  if (e->fig_head[lay] == NULL)
    e->fig_head[lay] = e->fig_tail[lay] = nf;
  else {
    e->fig_tail[lay]->next = nf;
    e->fig_tail[lay] = nf;
  }
}

void ezx_point_2d(ezx_t *e, int x, int y, const ezx_color_t *col)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FPOINT2D;
  nf->x0 = x;
  nf->y0 = y;
  nf->col = *col;

  figure_list_add_tail(e, nf);
}

void ezx_line_2d(ezx_t *e, int x0, int y0, int x1, int y1, const ezx_color_t *col,
		 int width)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FLINE2D;
  nf->x0 = x0;
  nf->y0 = y0;
  nf->x1 = x1;
  nf->y1 = y1;
  nf->col = *col;
  nf->width = width;
  
  figure_list_add_tail(e, nf);
}

void ezx_lines_2d(ezx_t *e, ezx_point2d_t *points, int npoints,
		  const ezx_color_t *col, int width)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FLINES2D;
  nf->points_2d = points;
  nf->npoints = npoints;
  nf->col = *col;
  nf->width = width;
  
  figure_list_add_tail(e, nf);
}

void ezx_poly_2d(ezx_t *e,ezx_point2d_t *points, int npoints,
		 const ezx_color_t *col)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FPOLY2D;
  nf->points_2d = points;
  nf->npoints = npoints;
  nf->col = *col;
  
  figure_list_add_tail(e, nf);
}

void ezx_arc_2d(ezx_t *e, int x0, int y0, int w, int h, double angle1,
		double angle2, const ezx_color_t *col, int width)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FARC2D;
  nf->x0 = x0;
  nf->y0 = y0;
  nf->x1 = w;
  nf->y1 = h;
  nf->angle1 = angle1;
  nf->angle2 = angle2;
  nf->col = *col;
  nf->width = width;

  figure_list_add_tail(e, nf);
}

void ezx_fillarc_2d(ezx_t *e, int x0, int y0, int w, int h, double angle1,
		    double angle2, const ezx_color_t *col)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FFILLEDARC2D;
  nf->x0 = x0;
  nf->y0 = y0;
  nf->x1 = w;
  nf->y1 = h;
  nf->angle1 = angle1;
  nf->angle2 = angle2;
  nf->col = *col;
  nf->width = 0;

  figure_list_add_tail(e, nf);
}

void ezx_line_3d(ezx_t *e, double x0, double y0, double z0, double x1,
		 double y1, double z1, const ezx_color_t *col, int width)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FLINE3D;
  nf->dx0 = x0;
  nf->dy0 = y0;
  nf->dz0 = z0;
  nf->dx1 = x1;
  nf->dy1 = y1;
  nf->dz1 = z1;
  nf->col = *col;
  nf->width = width;
  nf->next = NULL;

  figure_list_add_tail(e, nf);
}

void ezx_str_3d(ezx_t *e, double x0, double y0, double z0, char *str,
		const ezx_color_t *col)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FSTR3D;
  nf->dx0 = x0;
  nf->dy0 = y0;
  nf->dz0 = z0;
  nf->str = xcalloc(strlen(str) + 1, sizeof(char));
  strcpy(nf->str, str);
  nf->col = *col;
  nf->next = NULL;

  figure_list_add_tail(e, nf);
}

void ezx_str_2d(ezx_t *e, int x0, int y0, char *str, const ezx_color_t *col)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FSTR2D;
  nf->x0 = x0;
  nf->y0 = y0;
  nf->str = xcalloc(strlen(str) + 1, sizeof(char));
  strcpy(nf->str, str);
  nf->col = *col;
  nf->width = 0;
  nf->next = NULL;

  figure_list_add_tail(e, nf);
}

void ezx_fillrect_2d(ezx_t *e, int x0, int y0, int x1, int y1,
		     const ezx_color_t *col)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FFILLEDRECT2D;
  nf->x0 = x0;
  nf->y0 = y0;
  nf->x1 = x1;
  nf->y1 = y1;
  nf->col = *col;
  nf->width = 0;
  nf->next = NULL;

  figure_list_add_tail(e, nf);
}

void ezx_rect_2d(ezx_t *e, int x0, int y0, int x1, int y1, const ezx_color_t *col,
		 int width)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FRECT2D;
  nf->x0 = x0;
  nf->y0 = y0;
  nf->x1 = x1;
  nf->y1 = y1;
  nf->col = *col;
  nf->width = width;
  nf->next = NULL;

  figure_list_add_tail(e, nf);
}

void ezx_poly_3d(ezx_t *e, ezx_point3d_t *points, double hx, double hy,
		 double hz, int npoints, const ezx_color_t *col)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));
  int i;

  nf->type = FPOLY3D;
  nf->points_3d = points;
  nf->npoints = npoints;
  nf->col = *col;
  nf->next = NULL;
  nf->dx0 = hx;
  nf->dy0 = hy;
  nf->dz0 = hz;

  nf->dx1 = nf->dy1 = nf->dz1 = 0;
  for (i = 0; i < npoints; i++) {
    nf->dx1 += points[i].x;
    nf->dy1 += points[i].y;
    nf->dz1 += points[i].z;
  }

  nf->dx1 /= npoints;
  nf->dy1 /= npoints;
  nf->dz1 /= npoints;

  figure_list_add_tail(e, nf);
}

void ezx_fillcircle_2d(ezx_t *e, int x0, int y0, int r, const ezx_color_t *col)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FFILLEDCIRCLE2D;
  nf->x0 = x0;
  nf->y0 = y0;
  nf->r = r;
  nf->col = *col;
  nf->width = 0;
  nf->next = NULL;

  figure_list_add_tail(e, nf);
}

void ezx_circle_2d(ezx_t *e, int x0, int y0, int r, const ezx_color_t *col,
		   int width)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FCIRCLE2D;
  nf->x0 = x0;
  nf->y0 = y0;
  nf->r = r;
  nf->col = *col;
  nf->width = width;
  nf->next = NULL;

  figure_list_add_tail(e, nf);
}

void ezx_circle_3d(ezx_t *e, double x0, double y0, double z0, double r,
		   const ezx_color_t *col)
{
  ezx_figures *nf = xcalloc(1, sizeof(ezx_figures));

  nf->type = FFILLEDCIRCLE3D;
  nf->dx0 = x0;
  nf->dy0 = y0;
  nf->dz0 = z0;
  nf->dr = r;
  nf->col = *col;
  nf->next = NULL;

  figure_list_add_tail(e, nf);
}

void ezx_set_background(ezx_t *e, const ezx_color_t *col)
{
  e->bgcolor = *col;
}

static inline COLORREF ecol2rgb(ezx_color_t col)
{
  return RGB((BYTE)(col.r * 255), (BYTE)(col.g * 255), (BYTE)(col.b * 255));
}

void ezx_redraw(ezx_t * e)
{
  ezx_figures  *f;
  ezx_pfigures *p;
  ezx_pfigures *pf;

  HDC hdc = e->hdcMem;
  RECT rect;

  EnterCriticalSection(&e->cSect);

  GetClientRect(e->hWnd, &rect);
  SelectObject(hdc, CreateSolidBrush(ecol2rgb(e->bgcolor)));
  PatBlt(e->hdcMem, rect.left, rect.top, rect.right, rect.bottom, PATCOPY);
  DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));

  // z-sorting
  pf = NULL;
  for (f = e->fig_head[e->cur_layer]; f != NULL; f = f->next) {
    double z;
    ezx_pfigures **p;

    if (f->type != FPOLY3D) continue;

    z = getz(e, f->dx1, f->dy1, f->dz1);

    for (p = &pf; *p != NULL; p = &(*p)->next) {
      ezx_pfigures *np;

      if (z <= (*p)->z) continue;

      np = xmalloc(sizeof(ezx_pfigures));
      np->z = z;
      np->fig = f;
      np->next = *p;
      *p = np;
      break;
    }

    if (*p == NULL) {
      ezx_pfigures *np = xmalloc(sizeof(ezx_pfigures));
      np->z = z;
      np->fig = f;
      np->next = NULL;
      *p = np;
    }
  }

  for (p = pf; p != NULL; p = p->next) {
    ezx_figures *f = p->fig;
    double hx, hy, hz;
    double cl, br;
    int i;

    if (f->dx0 != 0 || f->dy0 != 0 || f->dz0 != 0) {
      hx = f->dx0;
      hy = f->dy0;
      hz = f->dz0;

      if (hx * (e->eye_x - f->dx1) +
	  hy * (e->eye_y - f->dy1) +
	  hz * (e->eye_z - f->dz1) < 0) {
	continue;
      }
    } else {
      for (i = 2;; i++) {
	double x0 = f->points_3d[1].x - f->points_3d[0].x;
	double y0 = f->points_3d[1].y - f->points_3d[0].y;
	double z0 = f->points_3d[1].z - f->points_3d[0].z;
	double x1 = f->points_3d[i].x - f->points_3d[0].x;
	double y1 = f->points_3d[i].y - f->points_3d[0].y;
	double z1 = f->points_3d[i].z - f->points_3d[0].z;

	hx = y0 * z1 - y1 * z0;
	hy = z0 * x1 - z1 * x0;
	hz = x0 * y1 - x1 * y0;

	if (hx != 0 || hy != 0 || hz != 0) break;
      }
    }

    cl = (e->light_x * f->dx0 + e->light_y * f->dy0 +
	  e->light_z * f->dz0) / sqrt(f->dx0 * f->dx0 +
					  f->dy0 * f->dy0 +
					  f->dz0 * f->dz0);
    if (cl < 0) {
      cl = -cl;
      f->dx0 = -f->dx0;
      f->dy0 = -f->dy0;
      f->dz0 = -f->dz0;
    }
    if (f->dx0 * (e->eye_x - f->dx1) +
	f->dy0 * (e->eye_y - f->dy1) +
	f->dz0 * (e->eye_z - f->dz1) < 0)
      cl = 0;
    br = cl * 0.6 + 0.3;

    POINT *points = xmalloc(sizeof(POINT) * f->npoints);
    for (i = 0; i < f->npoints; i++) {
      double sx0, sy0;
      ezx_c3d_to_2d(e, f->points_3d[i].x, f->points_3d[i].y,
		    f->points_3d[i].z, &sx0, &sy0);
      points[i].x = (int)sx0 + (e->size_x / 2);
      points[i].y = (int)sy0 + (e->size_y / 2);
    }

    SelectObject(hdc, CreateSolidBrush(ecol2rgb(f->col)));
    Polygon(hdc, points, f->npoints);
    DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));

    free(points);
  }

  {
    ezx_pfigures *np, *p;
    for (p = pf; p != NULL; p = np) {
      np = p->next;
      free(p);
    }

    pf = NULL;
  }

  {
    ezx_figures *f;
    int i;

    for (i = 0; i < EZX_NLAYER; i++) {
      for (f = e->fig_head[i]; f != NULL; f = f->next) {
	switch (f->type) {
	case FPOINT2D:
	  SetPixel(hdc, f->x0, f->y0, ecol2rgb(f->col));
	  break;
	case FLINE2D:
	  if (f->width <= 0) break;
	  SelectObject(hdc, CreatePen(PS_SOLID, f->width, ecol2rgb(f->col)));
	  MoveToEx(hdc, f->x0, f->y0, NULL);
	  LineTo(hdc, f->x1, f->y1);
	  DeleteObject(SelectObject(hdc, GetStockObject(NULL_PEN)));
	  break;
	case FLINES2D:
	  {
	    int j;
	    if (f->width <= 0) break;
	    POINT *points = xmalloc(sizeof(POINT) * f->npoints);
	    for (j = 0; j < f->npoints; j++) {
	      points[j].x = f->points_2d[j].x;
	      points[j].y = f->points_2d[j].y;
	    }
	    SelectObject(hdc, CreatePen(PS_SOLID, f->width, ecol2rgb(f->col)));
	    Polyline(hdc, points, f->npoints);
	    DeleteObject(SelectObject(hdc, GetStockObject(NULL_PEN)));
	    free(points);
	  }
	  break;
	case FPOLY2D:
	  {
	    int j;
	    POINT *points = xmalloc(sizeof(POINT) * f->npoints);
	    for (j = 0; j < f->npoints; j++) {
	      points[j].x = f->points_2d[j].x;
	      points[j].y = f->points_2d[j].y;
	    }
	    SelectObject(hdc, CreateSolidBrush(ecol2rgb(f->col)));
	    Polygon(hdc, points, f->npoints);
	    DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
	    free(points);
	  }
	  break;
	case FLINE3D:
	  {
	    if (f->width <= 0) break;
	    double sx0, sy0, sx1, sy1;
	    int x0, y0, x1, y1;
	    ezx_c3d_to_2d(e, f->dx0, f->dy0, f->dz0, &sx0, &sy0);
	    ezx_c3d_to_2d(e, f->dx1, f->dy1, f->dz1, &sx1, &sy1);
	    x0 = (int)sx0 + (e->size_x / 2);
	    y0 = (int)sy0 + (e->size_y / 2);
	    x1 = (int)sx1 + (e->size_x / 2);
	    y1 = (int)sy1 + (e->size_y / 2);
	    clip_line(&x0, &y0, &x1, &y1, e->size_x, e->size_y);
	    SelectObject(hdc, CreatePen(PS_SOLID, f->width, ecol2rgb(f->col)));
	    MoveToEx(hdc, x0, y0, NULL);
	    LineTo(hdc, x1, y1);
	    DeleteObject(SelectObject(hdc, GetStockObject(NULL_PEN)));
	  }
	  break;
	case FCIRCLE2D:
	  if (f->width <= 0) break;
	  SelectObject(hdc, CreatePen(PS_SOLID, f->width, ecol2rgb(f->col)));
	  Ellipse(hdc, f->x0 - f->r, f->y0 - f->r, f->x0 + f->r, f->y0 + f->r);
	  DeleteObject(SelectObject(hdc, GetStockObject(NULL_PEN)));
	  break;
	case FFILLEDCIRCLE2D:
	  SelectObject(hdc, CreateSolidBrush(ecol2rgb(f->col)));
	  Ellipse(hdc, f->x0 - f->r + 1, f->y0 - f->r + 1, f->x0 + f->r,
		  f->y0 + f->r);
	  DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
	  break;
	case FFILLEDCIRCLE3D:
	  {
	    double sx0, sy0;
	    ezx_c3d_to_2d(e, f->dx0, f->dy0, f->dz0, &sx0, &sy0);
	    SelectObject(hdc, CreateSolidBrush(ecol2rgb(f->col)));
	    Ellipse(hdc,
		    (int)sx0 + (e->size_x / 2) - (int)f->dr + 1,
		    (int)sy0 + (e->size_y / 2) - (int)f->dr + 1,
		    (int)sx0 + (e->size_x / 2) + (int)f->dr,
		    (int)sy0 + (e->size_y / 2) + (int)f->dr);
	    DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
	  }
	  break;
	case FSTR2D:
	  {
	    TEXTMETRIC tm;
	    GetTextMetrics(hdc, &tm);
	    SetTextColor(hdc, ecol2rgb(f->col));
	    TextOut(hdc, f->x0, f->y0 - tm.tmAscent, f->str, strlen(f->str));
	  }
	  break;
	case FSTR3D:
	  {
	    double sx0, sy0;
	    TEXTMETRIC tm;
	    ezx_c3d_to_2d(e, f->dx0, f->dy0, f->dz0, &sx0, &sy0);
	    GetTextMetrics(hdc, &tm);
	    SetTextColor(hdc, ecol2rgb(f->col));
	    TextOut(hdc, (int)sx0 + (e->size_x / 2),
		    (int)sy0 + (e->size_y / 2) - tm.tmAscent,
		    f->str, strlen(f->str));
	  }
	  break;
	case FRECT2D:
	  if (f->width <= 0) break;
	  SelectObject(hdc, CreatePen(PS_SOLID, f->width, ecol2rgb(f->col)));
	  Rectangle(hdc, f->x0, f->y0, f->x1 + 1, f->y1 + 1);
	  DeleteObject(SelectObject(hdc, GetStockObject(NULL_PEN)));
	  break;
	case FFILLEDRECT2D:
	  SelectObject(hdc, CreateSolidBrush(ecol2rgb(f->col)));
	  Rectangle(hdc, f->x0, f->y0, f->x1 + 1, f->y1 + 1);
	  DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
	  break;
	case FARC2D:
	  {
	    double r0, r1, t0, t1;
	    if (f->width <= 0) break;
	    r0 = f->x1 / 2;
	    r1 = f->y1 / 2;
	    t0 = f->angle1 * M_PI / 180.0;
	    t1 = t0 + f->angle2 * M_PI / 180.0;
	    SelectObject(hdc, CreatePen(PS_SOLID, f->width, ecol2rgb(f->col)));
	    Arc(hdc,
		(f->x0 - r0), (f->y0 - r1) ,
		(f->x0 + r0) + 1, (f->y0 + r1) + 1,
		(f->x0 + r0*cos(t0)), (f->y0 - r1*sin(t0)),
		(f->x0 + r0*cos(t1)), (f->y0 - r1*sin(t1)));
	    DeleteObject(SelectObject(hdc, GetStockObject(NULL_PEN)));
	  }
	  break;
	case FFILLEDARC2D:
	  {
	    double r0, r1, t0, t1;
	    r0 = f->x1 / 2;
	    r1 = f->y1 / 2;
	    t0 = f->angle1 * M_PI / 180.0;
	    t1 = t0 + f->angle2 * M_PI / 180.0;
	    SelectObject(hdc, CreateSolidBrush(ecol2rgb(f->col)));
	    Pie(hdc,
		(f->x0 - r0), (f->y0 - r1),
		(f->x0 + r0) + 2, (f->y0 + r1) + 2,
		(f->x0 + r0*cos(t0)), (f->y0 - r1*sin(t0)),
		(f->x0 + r0*cos(t1)), (f->y0 - r1*sin(t1)));
	    DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
	  }
	  break;
	case FPOLY3D:
	  break;
	}
      }
    }
  }

  InvalidateRect(e->hWnd, NULL, FALSE);

  LeaveCriticalSection(&e->cSect);

  Sleep(0);
}

static inline unsigned int get_state_mask(WPARAM wParam)
{
  unsigned int state = 0;

  if (wParam & MK_SHIFT) state |= EZX_SHIFT_MASK;
  if (wParam & MK_CONTROL) state |= EZX_CONTROL_MASK;
  if (wParam & MK_LBUTTON) state |= EZX_BUTTON_LMASK;
  if (wParam & MK_MBUTTON) state |= EZX_BUTTON_MMASK;
  if (wParam & MK_RBUTTON) state |= EZX_BUTTON_RMASK;

  return state;
}

static inline unsigned int get_current_state_mask()
{
  unsigned int state = 0;
  
  if (GetAsyncKeyState(VK_SHIFT) < 0) state |= EZX_SHIFT_MASK;
  if (GetAsyncKeyState(VK_CONTROL) < 0) state |= EZX_CONTROL_MASK;
  if (GetAsyncKeyState(VK_LBUTTON) < 0) state |= EZX_BUTTON_LMASK;
  if (GetAsyncKeyState(VK_MBUTTON) < 0) state |= EZX_BUTTON_MMASK;
  if (GetAsyncKeyState(VK_RBUTTON) < 0) state |= EZX_BUTTON_RMASK;

  return state;
}

static inline unsigned int get_message_state_mask()
{
  unsigned int state = 0;
  
  if (GetKeyState(VK_SHIFT) < 0) state |= EZX_SHIFT_MASK;
  if (GetKeyState(VK_CONTROL) < 0) state |= EZX_CONTROL_MASK;
  if (GetKeyState(VK_LBUTTON) < 0) state |= EZX_BUTTON_LMASK;
  if (GetKeyState(VK_MBUTTON) < 0) state |= EZX_BUTTON_MMASK;
  if (GetKeyState(VK_RBUTTON) < 0) state |= EZX_BUTTON_RMASK;

  return state;
}

static inline unsigned int get_key_code(WPARAM wParam, LPARAM lParam)
{
  unsigned int k = 0;
  static BYTE chars[2], keystate[256];

  if (wParam == VK_HOME) return EZX_KEY_HOME;
  else if (wParam == VK_LEFT) return EZX_KEY_LEFT;
  else if (wParam == VK_RIGHT) return EZX_KEY_RIGHT;
  else if (wParam == VK_UP) return EZX_KEY_UP;
  else if (wParam == VK_DOWN) return EZX_KEY_DOWN;
  else {
    lParam &= 0x7fffffff;
    GetKeyboardState(keystate);
    if (ToAscii(wParam, HIWORD(lParam), keystate, (WORD *)chars, 0) == 1)
      k = chars[0];
  }
  
  return k;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  ezx_t *e = (ezx_t *)GetWindowLong(hWnd, GWL_USERDATA);

  switch (message) {
  case WM_PAINT:
    {
      HDC hdc;
      PAINTSTRUCT ps;
      EnterCriticalSection(&e->cSect);
      hdc = BeginPaint(hWnd, &ps);
      if (!BitBlt(hdc, 0, 0, e->size_x, e->size_y, e->hdcMem, 0, 0, SRCCOPY))
	win32_error_exit("WndProc: BitBlt failed");
      EndPaint(hWnd, &ps);
      LeaveCriticalSection(&e->cSect);
    }
    return 0;
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_MOUSEWHEEL:
    {
      ezx_event_t *event = xmalloc(sizeof(ezx_event_t));
      if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP)
	event->button.b = EZX_BUTTON_LEFT;
      else if (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP)
	event->button.b = EZX_BUTTON_RIGHT;
      else if (message == WM_MBUTTONDOWN || message == WM_MBUTTONUP)
	event->button.b = EZX_BUTTON_MIDDLE;
      else {
	if ((short)HIWORD(wParam) >= 0) event->button.b = EZX_BUTTON_WHEELUP;
	else event->button.b = EZX_BUTTON_WHEELDOWN;
      }
      event->button.x = LOWORD(lParam);
      event->button.y = HIWORD(lParam);
      event->button.state = get_state_mask(wParam);
      if (message == WM_LBUTTONUP || message == WM_RBUTTONUP ||
	  message == WM_MBUTTONUP) {
	ReleaseCapture();
	event->button.type = EZX_BUTTON_RELEASE;
	event_queue_offer(e->event_queue, QID_OTHERS, event);
      } else {
	SetCapture(e->hWnd);
	event->button.type = EZX_BUTTON_PRESS;
	event_queue_offer(e->event_queue, QID_BUTTON_PRESS, event);
      }
    }
    return 0;
  case WM_KEYDOWN:
  case WM_KEYUP:
    {
      unsigned int k = get_key_code(wParam, lParam);
      if (k) {
	ezx_event_t *event = xmalloc(sizeof(ezx_event_t));
	DWORD pos = GetMessagePos();
	if (message == WM_KEYDOWN) event->key.type = EZX_KEY_PRESS;
	else event->key.type = EZX_KEY_RELEASE;
	event->key.k = k;
	event->key.x = GET_X_LPARAM(pos);
	event->key.y = GET_Y_LPARAM(pos);
	event->key.state = get_message_state_mask();
	event_queue_offer(e->event_queue, QID_OTHERS, event);
      }
    }
    return 0;
  case WM_MOUSEMOVE:
    if (wParam & MK_LBUTTON || wParam & MK_RBUTTON || wParam & MK_MBUTTON) {
      ezx_event_t *event = xmalloc(sizeof(ezx_event_t));
      event->motion.type = EZX_MOTION_NOTIFY;
      event->motion.x = LOWORD(lParam);
      event->motion.y = HIWORD(lParam);
      event->motion.state = get_state_mask(wParam);
      event_queue_offer(e->event_queue, QID_OTHERS, event);
    }
    return 0;
  case WM_CLOSE:
    {
      int quit;
      ezx_event_t *event = xmalloc(sizeof(ezx_event_t));
      event->type = EZX_CLOSE;
      event_queue_offer(e->event_queue, QID_OTHERS, event);
      EnterCriticalSection(&e->cSect);
      e->closed = 1;
      quit = e->quit;
      LeaveCriticalSection(&e->cSect);
      if (quit) return  DefWindowProc(hWnd, message, wParam, lParam);
      else return 0;
    }
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
}

static unsigned __stdcall window_thread(void *arg)
{
  ezx_t *e = (ezx_t *)arg;
  RECT rect;
  HDC hdc;
  MSG msg;
  BOOL bRet;
  
  rect.left = 0;
  rect.top = 0;
  rect.right = e->size_x-1;
  rect.bottom = e->size_y-1;
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
  
  e->hWnd = CreateWindow(class_name,
			 e->window_name,
			 WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
			 CW_USEDEFAULT,
			 CW_USEDEFAULT,
			 rect.right - rect.left,
			 rect.bottom - rect.top,
			 NULL, NULL, GetModuleHandle(NULL), NULL);
  if (!e->hWnd) win32_error_exit("window_thread: CreateWindow failed");

  SetWindowLong(e->hWnd, GWL_USERDATA, (long)e);
  
  hdc = GetDC(e->hWnd);
  e->hBitmap = CreateCompatibleBitmap(hdc, e->size_x, e->size_y);
  e->hdcMem = CreateCompatibleDC(hdc);
  SelectObject(e->hdcMem, e->hBitmap);
  ReleaseDC(e->hWnd, hdc);

  SetBkMode(e->hdcMem, TRANSPARENT);

  SelectObject(e->hdcMem, GetStockObject(NULL_PEN));
  SelectObject(e->hdcMem, GetStockObject(NULL_BRUSH));

  e->hFont = CreateFont(12, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH | FF_MODERN, NULL);
  SelectObject(e->hdcMem, e->hFont);
  
  ShowWindow(e->hWnd, SW_SHOWNORMAL);
  UpdateWindow(e->hWnd);

  if (!SetEvent(e->hWindowCreation)) 
    win32_error_exit("window_thread: SetEvent failed");

  while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
    if (bRet == -1) win32_error_exit("window_thread: GetMessage failed");
    else {
      //TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return 0;
}

static void register_window_class()
{
  static int first = 1;

  if (first) {
    WNDCLASSEX wcex;
    
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC)WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = class_name;
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassEx(&wcex);

    first = 0;
  }
}

void ezx_window_name(ezx_t * e, char *window_name)
{
  e->window_name = window_name;
  SetWindowText(e->hWnd, e->window_name);
}

int ezx_isclosed(ezx_t * e)
{
  int b;
  
  EnterCriticalSection(&e->cSect);
  b = e->closed;
  LeaveCriticalSection(&e->cSect);

  return b;
}

void ezx_quit(ezx_t * e)
{
  ezx_wipe(e);

  EnterCriticalSection(&e->cSect);
  e->quit = 1;
  LeaveCriticalSection(&e->cSect);

  SendMessage(e->hWnd, WM_CLOSE, 0, 0);
  if (WaitForSingleObject(e->hThread, INFINITE) != WAIT_OBJECT_0)
    win32_error_exit("ezx_quit: WaitForSingleObject failed");

  event_queue_free(e->event_queue);
  
  DeleteDC(e->hdcMem);
  DeleteObject(e->hFont);
  DeleteObject(e->hBitmap);
  CloseHandle(e->hWindowCreation);
  CloseHandle(e->hThread);
  DeleteCriticalSection(&e->cSect);
  free(e);
}

ezx_t *ezx_init(int size_x, int size_y, char *window_name)
{
  ezx_t *e;
  unsigned tid;

  e = xcalloc(1, sizeof(ezx_t));
  e->size_x = size_x;
  e->size_y = size_y;
  e->cur_layer = 0;
  e->bgcolor = ezx_white;
  e->scrdist = 100;
  e->mag = 20;
  e->closed = 0;
  e->window_name = window_name;
  e->event_queue = event_queue_new();
  e->hWindowCreation = CreateEvent(NULL, FALSE, FALSE, NULL);
  InitializeCriticalSection(&e->cSect);

  ezx_set_view_3d(e, 1000, 0, 0, 0, 0, 0, 5);
  ezx_set_light_3d(e, 1000, 900, 800);

  register_window_class();

  e->hThread = (HANDLE)_beginthreadex(NULL, 0, window_thread, e, 0, &tid);
  if (!e->hThread) sys_error_exit("ezx_init: _beginthreadex failed");

  SetThreadPriority(e->hThread, THREAD_PRIORITY_ABOVE_NORMAL);

  if (WaitForSingleObject(e->hWindowCreation, INFINITE) != WAIT_OBJECT_0)
    win32_error_exit("ezx_init: WaitForSingleObject failed");

  ezx_redraw(e);

  return e;
}

int ezx_sensebutton(ezx_t *e, int *x, int *y)
{
  POINT pos;
  unsigned int s;

  GetCursorPos(&pos);
  s = get_current_state_mask();
  ScreenToClient(e->hWnd, &pos);
  if (x != NULL) *x = pos.x;
  if (y != NULL) *y = pos.y;

  return s;
}

int ezx_pushbutton(ezx_t *e, int *x, int *y)
{
  ezx_event_t *event = event_queue_remove(e->event_queue, QID_BUTTON_PRESS);
  int b = event->button.b;

  if (x != NULL) *x = event->button.x;
  if (y != NULL) *y = event->button.y;

  free(event);

  return b;
}

void ezx_next_event(ezx_t *e, ezx_event_t *ezx_event)
{
  ezx_event_t *event = event_queue_remove_oldest(e->event_queue);

  if (ezx_event) *ezx_event = *event;
  
  free(event);
}
