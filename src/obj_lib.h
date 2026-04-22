/* Written by Tejas Sethi
 * https://github.com/tejas25a/
 *
 * Supported file types - OBJ
 */
#ifndef OBJ_LIB_H
#define OBJ_LIB_H

#include <stddef.h>
#include <stdint.h>

/* ─── one face vertex: indices into attrib arrays (0-based, -1 = absent) ── */

typedef struct {
  int v;   /* position index */
  int vt;  /* texcoord index */
  int vn;  /* normal index   */
} ObjIndex;

/* ─── raw attribute storage ─────────────────────────────────────────────── */

typedef struct {
  float  *vertices;
  size_t  verticesCount;   /* number of xyz triples */
  float  *normals;
  size_t  normalsCount;    /* number of xyz triples */
  float  *texcoords;
  size_t  texcoordsCount;  /* number of uv pairs    */
} ObjAttrib;

/* ─── a mesh (one per object/group in the file) ─────────────────────────── */

typedef struct {
  char      name[128];
  ObjIndex *indices;
  size_t    indexCount;
} ObjMesh;

typedef struct {
  ObjMesh *meshes;
  size_t   meshCount;
} ObjShape;

/* ─── internal dynamic arrays (used in obj_lib.c only) ──────────────────── */

typedef struct { float    *data; size_t len, cap; } daFloat;
typedef struct { ObjIndex *data; size_t len, cap; } daIndex;

/* ─── public API ─────────────────────────────────────────────────────────── */

int  loadObj(ObjAttrib *attrib, ObjShape *shape, const char *filename);
void freeObj(ObjAttrib *attrib, ObjShape *shape);

#endif /* OBJ_LIB_H */
