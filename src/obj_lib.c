/* Written by Tejas Sethi
 * https://github.com/tejas25a/
 *
 * Supported file types - OBJ
 */
#ifndef OBJ_LIB
#define OBJ_LIB

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif /* ifndef OBJ_LIB */

#include "obj_lib.h"

static void daFloatPush(daFloat *a, float v) {
  if (a->len == a->cap) {
    a->cap = a->cap ? a->cap * 2 : 64;
    a->data = realloc(a->data, a->cap * sizeof(float));
  }
  a->data[a->len++] = v;
}

static void daIndexPush(daIndex *a, ObjIndex v) {
  if (a->len == a->cap) {
    a->cap = a->cap ? a->cap * 2 : 64;
    a->data = realloc(a->data, a->cap * sizeof(ObjIndex));
  }
  a->data[a->len++] = v;
}

static ObjIndex parseFaceToken(const char *tok) {
  ObjIndex idx = {-1, -1, -1};
  int v = 0, vt = 0, vn = 0;

  if (sscanf(tok, "%d/%d/%d", &v, &vt, &vn) == 3) {
    // v/vt/vn
  } else if (sscanf(tok, "%d//%d", &v, &vn) == 2) {
    // v//vn
  } else if (sscanf(tok, "%d/%d", &v, &vt) == 2) {
    // v/vt
  } else {
    sscanf(tok, "%d", &v);
    // v only
  }

  if (v != 0)
    idx.v = v > 0 ? v - 1 : 0;
  if (vt != 0)
    idx.vt = vt > 0 ? vt - 1 : 0;
  if (vn != 0)
    idx.vn = vn > 0 ? vn - 1 : 0;
  return idx;
}

static void flushMesh(ObjShape *shape, daIndex *cur, const char *name) {
  if (cur->len == 0)
    return;

  shape->meshes =
      realloc(shape->meshes, (shape->meshCount + 1) * sizeof(ObjMesh));
  ObjMesh *m = &shape->meshes[shape->meshCount++];

  strncpy(m->name, name, sizeof(m->name) - 1);
  m->name[sizeof(m->name) - 1] = '\0';
  m->indices = cur->data;
  m->indexCount = cur->len;

  cur->data = NULL;
  cur->len = 0;
  cur->cap = 0;
}

int loadObj(ObjAttrib *attrib, ObjShape *shape, const char *filename) {
  memset(attrib, 0, sizeof(*attrib));
  memset(shape, 0, sizeof(*shape));

  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Failed to open OBJ file: %s\n", filename);
    return 1;
  }

  daFloat pos = {0};
  daFloat norm = {0};
  daFloat tex = {0};
  daIndex cur = {0};

  char line[512];
  char curName[128] = "default";

  while (fgets(line, sizeof(line), file)) {
    line[strcspn(line, "\r\n")] = '\0';

    if (line[0] == '#' || line[0] == '\0')
      continue;

    // positions
    if (line[0] == 'v' && line[1] == ' ') {
      float x, y, z;
      if (sscanf(line + 2, "%f %f %f", &x, &y, &z) == 3) {
        daFloatPush(&pos, x);
        daFloatPush(&pos, y);
        daFloatPush(&pos, z);
      }

      // normals
    } else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
      float x, y, z;
      if (sscanf(line + 3, "%f %f %f", &x, &y, &z) == 3) {
        daFloatPush(&norm, x);
        daFloatPush(&norm, y);
        daFloatPush(&norm, z);
      }

      // texcoords
    } else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
      float u, v;
      if (sscanf(line + 3, "%f %f", &u, &v) == 2) {
        daFloatPush(&tex, u);
        daFloatPush(&tex, v);
      }

      // object / group -> new mesh
    } else if ((line[0] == 'o' || line[0] == 'g') && line[1] == ' ') {
      flushMesh(shape, &cur, curName);
      strncpy(curName, line + 2, sizeof(curName) - 1);
      curName[sizeof(curName) - 1] = '\0';

      // faces
    } else if (line[0] == 'f' && line[1] == ' ') {
      char copy[512];
      strncpy(copy, line + 2, sizeof(copy) - 1);
      char *tokens[64];
      int n = 0;
      char *tok = strtok(copy, " ");
      while (tok && n < 64) {
        tokens[n++] = tok;
        tok = strtok(NULL, " ");
      }
      // fan triangulation for quads and n-gons
      for (int i = 1; i + 1 < n; i++) {
        daIndexPush(&cur, parseFaceToken(tokens[0]));
        daIndexPush(&cur, parseFaceToken(tokens[i]));
        daIndexPush(&cur, parseFaceToken(tokens[i + 1]));
      }

      // material library
    } else if (strncmp(line, "mtllib ", 7) == 0) {
      printf("mtllib: %s\n", line + 7);

      // usemtl
    } else if (strncmp(line, "usemtl ", 7) == 0) {
      printf("usemtl: %s\n", line + 7);
    }
  }

  flushMesh(shape, &cur, curName);
  fclose(file);

  attrib->vertices = pos.data;
  attrib->verticesCount = pos.len / 3;
  attrib->normals = norm.data;
  attrib->normalsCount = norm.len / 3;
  attrib->texcoords = tex.data;
  attrib->texcoordsCount = tex.len / 2;

  return 0;
}

void freeObj(ObjAttrib *attrib, ObjShape *shape) {
  free(attrib->vertices);
  free(attrib->normals);
  free(attrib->texcoords);
  memset(attrib, 0, sizeof(*attrib));

  for (size_t i = 0; i < shape->meshCount; i++) {
    free(shape->meshes[i].indices);
  }
  free(shape->meshes);
  memset(shape, 0, sizeof(*shape));
}
