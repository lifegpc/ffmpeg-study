#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "zipfile.h"
#include <malloc.h>

typedef struct ZipFile {
    PyObject* zip;
} ZipFile;

ZipFile* open_zipfile(const char* fn) {
    if (!fn) {
        return NULL;
    }
    ZipFile* f = NULL;
    PyObject* zipfile = NULL, * zf = NULL, * pargs = NULL;
    zipfile = PyImport_ImportModule("zipfile"); // import zipfile
    if (!zipfile) goto end;
    zf = PyObject_GetAttrString(zipfile, "ZipFile"); // zf = zipfile.ZipFile
    if (!zf) goto end;
    pargs = Py_BuildValue("(s)", fn);
    if (!pargs) goto end;
    f = malloc(sizeof(ZipFile));
    if (!f) goto end;
    memset(f, 0, sizeof(ZipFile));
    f->zip = PyObject_Call(zf, pargs, NULL);  // zf(fn)
    if (!f->zip) {
        free(f);
        f = NULL;
        goto end;
    }
end:
    Py_XDECREF(zipfile);
    Py_XDECREF(zf);
    Py_XDECREF(pargs);
    return f;
}

MemFile* zipfile_get(ZipFile* s, const char* name) {
    if (!s || !name) return NULL;
    PyObject* re = NULL, * pargs = NULL, * read = NULL;
    MemFile* f = NULL;
    pargs = Py_BuildValue("(s)", name);
    if (!pargs) goto end;
    read = PyObject_GetAttrString(s->zip, "read");
    if (!read) goto end;
    re = PyObject_Call(read, pargs, NULL);
    if (!re || !PyBytes_Check(re)) goto end;
    f = new_memfile(PyBytes_AsString(re), PyBytes_Size(re));
end:
    Py_XDECREF(re);
    Py_XDECREF(pargs);
    Py_XDECREF(read);
    return f;
}

void free_zipfile(ZipFile* f) {
    Py_XDECREF(f->zip);
    free(f);
}
