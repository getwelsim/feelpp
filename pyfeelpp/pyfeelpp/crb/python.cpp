//! -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t  -*- vim:fenc=utf-8:ft=cpp:et:sw=4:ts=4:sts=4
//!
//! This file is part of the Feel++ library
//!
//! This library is free software; you can redistribute it and/or
//! modify it under the terms of the GNU Lesser General Public
//! License as published by the Free Software Foundation; either
//! version 2.1 of the License, or (at your option) any later version.
//!
//! This library is distributed in the hope that it will be useful,
//! but WITHOUT ANY WARRANTY; without even the implied warranty of
//! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//! Lesser General Public License for more details.
//!
//! You should have received a copy of the GNU Lesser General Public
//! License along with this library; if not, write to the Free Software
//! Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//!
//! @file
//! @author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
//! @date 14 Jun 2017
//! @copyright 2017 Feel++ Consortium
//!
#include <vector>

#include <Eigen/Core>
#include <boost/shared_ptr.hpp>
#include <pybind11/pybind11.h>
//#include <pybind11/eigen.h>
//#include <pybind11/numpy.h>


#include <feel/feelcore/feel.hpp>
#include <feel/feelcrb/crbdata.hpp>
#include <feel/feelcrb/parameterspace.hpp>
#include <feel/feelcrb/crbplugin_interface.hpp>
#include <feel/feelcrb/options.hpp>

namespace py = pybind11;
PYBIND11_DECLARE_HOLDER_TYPE(T, boost::shared_ptr<T>);
using namespace Feel;

namespace Eigen {
namespace internal {
template<>
struct traits<ParameterSpaceX::Element>: traits<Eigen::VectorXd>
{
};
}
}

using VectorXr=Eigen::Matrix<Real,Eigen::Dynamic,1>;
    
template <class T>
void vector_setitem(std::vector<T>& v, int index, T value)
{
    if (index >= 0 && index < v.size()) {
        v[index] = value;
    }
    else {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        //throw_error_already_set();
    }
}

template <class T>
T vector_getitem(std::vector<T> &v, int index)
{
    if (index >= 0 && index < v.size()) {
        return v[index];
    }
    else {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        //throw_error_already_set();
    }
}
void IndexError() { PyErr_SetString(PyExc_IndexError, "Index out of range"); }
template<class T>
struct std_item
{
    typedef typename T::value_type V;
    static V const& get(T const& x, int i)
        {
            if( i<0 ) i+=x.size();
            if( i>=0 && i<x.size() ) return x[i];
            IndexError();
        }
    static void set(T & x, int i, V const& v)
        {
            if( i<0 ) i+=x.size();
            if( i>=0 && i<x.size() ) x[i]=v;
            else IndexError();
        }
    static void del(T& x, int i)
        {
            if( i<0 ) i+=x.size();
            if( i>=0 && i<x.size() ) x.erase(i);
            else IndexError();
        }
    static void add(T& x, V const& v)
        {
            x.push_back(v);
        }
    static size_t size(T const& x)
        {
            return x.size();
        }
    
};
template<class T>
struct str_item
{
    static std::string to_string( T const& t )
        {
            std::ostringstream os;
            os << t;
            return os.str();
        }
};


po::options_description
makeCRBOptions()
{
    po::options_description opts( "crb online run lib options" );
    opts.add(crbOptions())
        .add(crbSEROptions())
        .add(eimOptions())
        .add(podOptions())
        .add(backend_options("backend-primal"))
        .add(backend_options("backend-dual"))
        .add(backend_options("backend-l2"))
        ;
    return opts;
}

class CRBPluginAPIWrap : public CRBPluginAPI
{
public:
    std::string const& name() const override
        {
            PYBIND11_OVERLOAD_PURE(std::string const&, CRBPluginAPI, name, );
        }
    
    void loadDB( std::string db ) override
        {
            PYBIND11_OVERLOAD_PURE( void, CRBPluginAPI, loadDB, db );
        }
    boost::shared_ptr<ParameterSpaceX> parameterSpace() const override
        {
            PYBIND11_OVERLOAD_PURE(boost::shared_ptr<ParameterSpaceX>, CRBPluginAPI, parameterSpace,  );
        }
    CRBResults run( ParameterSpaceX::Element const& mu, 
                    double eps , int N, bool print_rb_matrix ) const override
        {
            std::cout << "coucou\n";
            PYBIND11_OVERLOAD(CRBResults, CRBPluginAPI, run, mu, eps, N, print_rb_matrix ); 
        }
    void initExporter() override
        {
            PYBIND11_OVERLOAD_PURE(void, CRBPluginAPI, initExporter, );
        }
    void exportField( std::string const & name, CRBResults const& results ) override
        {
            PYBIND11_OVERLOAD_PURE(void, CRBPluginAPI, exportField, name, results );
            
        }
    void saveExporter() const override { PYBIND11_OVERLOAD_PURE(void, CRBPluginAPI, saveExporter,  ); }
protected:
    void setName( std::string const& n ) override { PYBIND11_OVERLOAD_PURE(void, CRBPluginAPI, setName, n ); }
};



namespace py = pybind11;
PYBIND11_MODULE( _crb, m )
{
    m.def("makeCRBOptions", &makeCRBOptions, "Create CRB Options" );
    m.def("factoryCRBPlugin", &factoryCRBPlugin, "Factory for CRB plugins" );

    py::class_<CRBResults>(m,"CRBResults")
        .def("output", &CRBResults::output)
        .def("errorBound", &CRBResults::errorbound)
        ;
    using ElementP = ParameterSpaceX::Element;
    py::class_<ElementP>(m,"ParameterSpaceElement");
    //.def("__repr__",&str_item<ParameterSpaceX::Element>::to_string)
    //.def("__str__", &str_item<ParameterSpaceX::Element>::to_string);


    //!
    //! Sampling wrapping
    //!
    
    py::class_<ParameterSpaceX::Sampling,boost::shared_ptr<ParameterSpaceX::Sampling>>(m,"ParameterSpaceSampling")
        .def(py::init<boost::shared_ptr<ParameterSpaceX>,int,boost::shared_ptr<ParameterSpaceX::Sampling>>())
        .def("sampling",&ParameterSpaceX::Sampling::sampling)
        .def("__getitem__", &std_item<ParameterSpaceX::Sampling>::get,py::return_value_policy::reference)
        .def("__setitem__", &std_item<ParameterSpaceX::Sampling>::set)
        .def("__len__", &std_item<ParameterSpaceX::Sampling>::size)
        ;

    //!
    //! Parameter Space
    //!
    py::class_<ParameterSpaceX,boost::shared_ptr<ParameterSpaceX>>(m,"ParameterSpace")
        .def( py::init<>() )
        .def("sampling", &ParameterSpaceX::sampling)
        .def("element", &ParameterSpaceX::element)
        //.def("New", &ParameterSpaceX::New, ParameterSpaceX_New_overloads(args("dim", "WorldComm"), "New")).staticmethod("New")
        .def_static("create",&ParameterSpaceX::create )
        ;
#if 1
#if 0

    // taken from boost.python libs/python/test/shared_ptr.cpp:
    // This is the ugliness required to register a to-python converter
    // for shared_ptr<A>.
    objects::class_value_wrapper<
        boost::shared_ptr<CRBPluginAPI>
        , objects::make_ptr_instance<CRBPluginAPI,
                                     objects::pointer_holder<boost::shared_ptr<CRBPluginAPI>,CRBPluginAPI> >
        >();
#endif
    
    py::class_<CRBPluginAPI,CRBPluginAPIWrap,boost::shared_ptr<CRBPluginAPI>>(m,"CRBPlugin")
        //.def(py::init<>())
        .def("name",&CRBPluginAPI::name,py::return_value_policy::reference)
        //.def("setName",pure_virtual(&CRBPluginAPI::setName))
        .def("loadDB",&CRBPluginAPI::loadDB)
        .def("initExporter",&CRBPluginAPI::initExporter)
        .def("saveExporter",&CRBPluginAPI::saveExporter)
        .def("exportField",&CRBPluginAPI::exportField)

        .def("run",py::overload_cast<ParameterSpaceX::Element const&,double,int,bool>(&CRBPluginAPI::run, py::const_));
        //.def("run",&CRBPluginAPI::run)
    
#endif
}