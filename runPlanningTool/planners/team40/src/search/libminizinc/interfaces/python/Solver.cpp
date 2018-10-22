/*  Python Interface for MiniZinc constraint modelling
 *  Author:
 *     Tai Tran <tai.tran@student.adelaide.edu.au>
 *  Supervisor:
 *     Guido Tack <guido.tack@monash.edu>
 */

#include "Solver.h"

static PyObject*
PyMznSolver_get_value_helper(PyMznSolver* self, const char* const name)
{
  for (unsigned int i=0; i<self->_m->size(); ++i) {
    if (VarDeclI* vdi = (*(self->_m))[i]->dyn_cast<VarDeclI>()) {
      if (strcmp(vdi->e()->id()->str().c_str(), name) == 0) {
        GCLock Lock;
        if (PyObject* PyValue = minizinc_to_python(vdi->e()))
          return PyValue;
        else {
          char buffer[50];
          sprintf(buffer, "Cannot retrieve the value of '%s'", name);
          PyErr_SetString(PyExc_RuntimeError, buffer);
          return NULL;
        }
      }
    }
  }
  char buffer[50];
  sprintf(buffer, "'%s' not found", name);
  PyErr_SetString(PyExc_RuntimeError, buffer);
  return NULL;
}

static PyObject* 
PyMznSolver_get_value(PyMznSolver* self, PyObject* args) {
  const char* name;
  PyObject* obj;
  if (!(self->_m)) {
    PyErr_SetString(PyExc_RuntimeError, "No model (maybe you need to call Model.next() first");
    return NULL;
  }
  if (!PyArg_ParseTuple(args, "O", &obj)) {
    PyErr_SetString(PyExc_TypeError,"Accept 1 argument of strings or list/tuple of strings");
    return NULL;
  }
  if (PyUnicode_Check(obj)) {
    name = PyUnicode_AsUTF8(obj);
    return PyMznSolver_get_value_helper(self, name);;
  } else 
  // XXX: INEFFICIENT function to retrieve values, consider optimize it later
  // Python Dictionary would be good
    if (PyList_Check(obj)) {
      Py_ssize_t n = PyList_GET_SIZE(obj);
      PyObject* ret = PyList_New(n);
      for (Py_ssize_t i=0; i!=n; ++i) {
        PyObject* item = PyList_GET_ITEM(obj, i);
        if (!PyUnicode_Check(item)) {
          Py_DECREF(ret);
          PyErr_SetString(PyExc_RuntimeError,"Elements must be strings");
          return NULL;
        }
        name = PyUnicode_AsUTF8(item);
        PyObject* value = PyMznSolver_get_value_helper(self, name);
        if (value == NULL) {
          Py_DECREF(ret);
          return NULL;
        }
        PyList_SET_ITEM(ret,i,value);
      }
      return ret;
    } else if (PyTuple_Check(obj)) {
      Py_ssize_t n = PyTuple_GET_SIZE(obj);
      PyObject* ret = PyTuple_New(n);
      for (Py_ssize_t i=0; i!=n; ++i) {
        PyObject* item = PyTuple_GET_ITEM(obj, i);
        if (!PyUnicode_Check(item)) {
          Py_DECREF(ret);
          PyErr_SetString(PyExc_RuntimeError,"Elements must be strings");
          return NULL;
        }
        name = PyUnicode_AsUTF8(item);
        PyObject* value = PyMznSolver_get_value_helper(self, name);
        if (value == NULL) {
          Py_DECREF(ret);
          return NULL;
        }
        PyTuple_SET_ITEM(ret,i,value);
      }
      return ret;
    } else {
      PyErr_SetString(PyExc_TypeError, "Object must be a string or a list/tuple of strings");
      return NULL;
    }
}


PyObject*
PyMznSolver::next()
{
  if (solver==NULL)
    throw runtime_error("Solver Object not found");
  GCLock lock;
  SolverInstance::Status status = solver->solve();
  if (status == SolverInstance::SAT || status == SolverInstance::OPT) {
    _m = env->output();
    Py_RETURN_NONE; 
  }
  if (_m == NULL)
    return PyUnicode_FromString("Unsatisfied");
  else
    return PyUnicode_FromString("Reached last solution");
}


static void
PyMznSolver_dealloc(PyMznSolver* self)
{
  if (self->env)
    delete self->env;
  if (self->solver)
    delete self->solver;
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

static PyObject*
PyMznSolver_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  PyMznSolver* self = reinterpret_cast<PyMznSolver*>(type->tp_alloc(type,0));
  self->solver = NULL;
  self->_m = NULL;
  self->env = NULL;
  return reinterpret_cast<PyObject*>(self);
}

static int
PyMznSolver_init(PyMznSolver* self, PyObject* args)
{
  return 0;
}

static PyObject*
PyMznSolver_next(PyMznSolver *self)
{
  return self->next();
}
