#include <stdio.h>
#include <stdbool.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include "../external/cubiomes/biomenoise.h"
#include "../external/cubiomes/generator.h"

extern PyTypeObject SurfaceNoiseType;


typedef struct {
    PyObject_HEAD
    Generator generator;
} GeneratorObject;

extern PyTypeObject GeneratorType;

#define GENERATOR_OBJECT_TYPE "O!"

static int Generator_traverse(GeneratorObject *self, visitproc visit, void *arg) {
    return 0;
}

static int Generator_clear(GeneratorObject *self) {
    return 0;
}

static void Generator_dealloc(GeneratorObject *self) {
    PyObject_GC_UnTrack(self);
    Generator_clear(self);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *Generator_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    GeneratorObject *self;
    self = (GeneratorObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->generator = (Generator){0};
        setupGenerator(&self->generator, MC_1_18, 0);
    }
    return (PyObject *) self;
}

static int Generator_init(GeneratorObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"version", "flags", NULL};

    int version;
    uint32_t flags;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iI", kwlist, &version, &flags)) {
        return -1;
    }

    self->generator = (Generator){0};
    setupGenerator(&self->generator, version, flags);

    return 0;
}

static PyMemberDef Generator_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *Generator_apply_seed(GeneratorObject *self, PyObject *args) {
    uint64_t seed;
    int dimension;

    if (!PyArg_ParseTuple(args, "Ki", &seed, &dimension)) {
        return NULL;
    }

    applySeed(&self->generator, dimension, seed);
    Py_RETURN_NONE;
}

static PyObject *Generator_get_biome_at(GeneratorObject *self, PyObject *args) {
    int scale, x, y, z;
    
    if (!PyArg_ParseTuple(args, "iiii", &scale, &x, &y, &z)) {
        return NULL;
    }

    int id = getBiomeAt(&self->generator, scale, x, y, z);

    return PyLong_FromLong(id);
}

static PyObject *Generator_gen_biomes(GeneratorObject *self, PyObject *args) {
    int x, y, z, sx, sy, sz, scale;
    
    // The C function is now configured to expect seven integers.
    if (!PyArg_ParseTuple(args, "iiiiiii", &x, &y, &z, &sx, &sy, &sz, &scale)) {
        return NULL;
    }

    Range r;
    r.scale = scale;
    r.x = x;
    r.y = y;
    r.z = z;
    r.sx = sx;
    r.sy = sy;
    r.sz = sz;

    int *biomeIds = allocCache(&self->generator, r);
    if (!biomeIds) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory for biome cache.");
        return NULL;
    }

    genBiomes(&self->generator, biomeIds, r);

    size_t len = getMinCacheSize(&self->generator, r.scale, r.sx, r.sy, r.sz);
    PyObject *list = PyList_New(len);
    if (!list) {
        free(biomeIds);
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        PyObject *biome_py_obj = PyLong_FromLong(biomeIds[i]);
        if (!biome_py_obj) {
            Py_DECREF(list);
            free(biomeIds);
            return NULL;
        }
        PyList_SetItem(list, i, biome_py_obj);
    }
    
    free(biomeIds);
    return list;
}

static PyObject *Generator_is_viable_structure_pos(GeneratorObject *self, PyObject *args) {
    int structure, x, z;
    uint32_t flags;

    if (!PyArg_ParseTuple(args, "iiii", &structure, &x, &z, &flags)) {
        return NULL;
    }

    int ret = isViableStructurePos(structure, &self->generator, x, z, flags);
    return PyBool_FromLong(ret);
}

static PyObject *Generator_map_approx_height(GeneratorObject *self, PyObject *args) {
    PyObject *sn_obj;
    int x, z, w, h;
    
    if (!PyArg_ParseTuple(args, "O!iiii", &SurfaceNoiseType, &sn_obj, &x, &z, &w, &h)) {
        return NULL;
    }
    
    SurfaceNoiseObject *sn = (SurfaceNoiseObject *)sn_obj;

    float *y = (float *)malloc(w * h * sizeof(float));
    int *ids = (int *)malloc(w * h * sizeof(int));
    
    if (!y || !ids) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory.");
        free(y);
        free(ids);
        return NULL;
    }
	
    int result = mapApproxHeight(y, ids, &self->generator, &sn->noise, x, z, w, h);

    if (result != 0) {
        PyErr_SetString(PyExc_RuntimeError, "mapApproxHeight returned a non-zero value, indicating an error.");
        free(y);
        free(ids);
        return NULL;
    }
    PyObject *y_list = PyList_New(w * h);
    PyObject *ids_list = PyList_New(w * h);
    
    for (int i = 0; i < w * h; i++) {
        PyList_SetItem(y_list, i, PyFloat_FromDouble(y[i]));
        PyList_SetItem(ids_list, i, PyLong_FromLong(ids[i]));
    }
    
    free(y);
    free(ids);
    
    return PyTuple_Pack(2, y_list, ids_list);
}

static PyMethodDef Generator_methods[] = {
    {"apply_seed", (PyCFunction) Generator_apply_seed, METH_VARARGS, "Applies a seed to the generator"},
    {"get_biome_at", (PyCFunction) Generator_get_biome_at, METH_VARARGS, "Get the biome at the specified location"},
    {"gen_biomes", (PyCFunction) Generator_gen_biomes, METH_VARARGS, "Get the biome at the specified location"},
    {"is_viable_structure_pos", (PyCFunction) Generator_is_viable_structure_pos, METH_VARARGS, "Get the biome at the specified location"},
    {"map_approx_height", (PyCFunction)Generator_map_approx_height, METH_VARARGS, "Maps an approximation of the Overworld surface height."},
    {NULL}  /* Sentinel */
};

static PyTypeObject GeneratorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pybiomes.Generator",
    .tp_doc = "Generator objects",
    .tp_basicsize = sizeof(GeneratorObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_new = Generator_new,
    .tp_init = (initproc) Generator_init,
    .tp_dealloc = (destructor) Generator_dealloc,
    .tp_traverse = (traverseproc) Generator_traverse,
    .tp_clear = (inquiry) Generator_clear,
    .tp_members = Generator_members,
    .tp_methods = Generator_methods,
};
