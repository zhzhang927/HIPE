//READ LICENSE BEFORE ANY USAGE
/* Copyright (C) 2018  Damien DUBUC ddubuc@aneo.fr (ANEO S.A.S)
 *  Team Contact : hipe@aneo.fr
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *  
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *  
 *  In addition, we kindly ask you to acknowledge ANEO and its authors in any program 
 *  or publication in which you use HIPE. You are not required to do so; it is up to your 
 *  common sense to decide whether you want to comply with this request or not.
 *  
 *  Non-free versions of HIPE are available under terms different from those of the General 
 *  Public License. e.g. they do not require you to accompany any object code using HIPE 
 *  with the corresponding source code. Following the new licensing any change request from 
 *  contributors to ANEO must accept terms of re-license by a general announcement. 
 *  For these alternative terms you must request a license from ANEO S.A.S Company 
 *  Licensing Office. Users and or developers interested in such a license should 
 *  contact us (hipe@aneo.fr) for more information.
 */

#include <boost/algorithm/string.hpp>

#include <filter/algos/PythonFilter.h>
#include <core/HipeStatus.h>
#include <core/misc.h>
#include <core/python/pyThreadSupport.h>
#include <corefilter/tools/Localenv.h>
#include "data/FileVideoInput.h"
#include <coredata/Data.h>
#include "pydata/pyShapeData.h"
#include "pydata/pyImageArrayData.h"
#include "data/ShapeData.h"
#include <glog/logging.h>
#include "data/pyObjectData.h"


extern "C" {
#include <Python.h>
}

// #define WINDOWS  /* uncomment this line to use it for windows.*/ 

#include<iostream>

#pragma warning(push, 0)
#include <boost/python.hpp>
#pragma warning(pop)

#include <pydata/pyImageData.h>

namespace filter
{
	namespace algos
	{
		class Hipe_PyEnsureGIL
		{
		public:
			Hipe_PyEnsureGIL() :
				_state(PyGILState_Ensure())
			{
			}

			~Hipe_PyEnsureGIL()
			{
				PyGILState_Release(_state);
			}

		private:
			PyGILState_STATE _state;
		};

		class Hipe_PyAllowThreads
		{
		public:
			Hipe_PyAllowThreads() :
				_state(PyEval_SaveThread())
			{
			}

			~Hipe_PyAllowThreads()
			{
				PyEval_RestoreThread(_state);
			}

		private:
			PyThreadState* _state;
		};

		// decode a Python exception into a string
		std::string handle_pyerror()
		{
			using namespace boost::python;
			using namespace boost;

			PyObject *exc, *val, *tb;
			object formatted_list, formatted;
			PyErr_Fetch(&exc, &val, &tb);
			PyErr_NormalizeException(&exc, &val, &tb);
			handle<> hexc(exc), hval(allow_null(val)), htb(allow_null(tb));
			object traceback(import("traceback"));
			if (!tb)
			{
				object format_exception_only(traceback.attr("format_exception_only"));
				formatted_list = format_exception_only(hexc, hval);
			}
			else
			{
				object format_exception(traceback.attr("format_exception"));
				formatted_list = format_exception(hexc, hval, htb);
			}
			formatted = str("\n").join(formatted_list);
			return extract<std::string>(formatted);
		}

		void PythonFilter::add_python_path(const std::string& python_path)
		{
			std::vector<std::string> results;

			boost::split(results, python_path, [](char c) { return c == ';'; });
			
			for (auto p : results)
			{
				std::ostringstream python_path_builder;
				python_path_builder << "sys.path.insert(0, r'" << p << "'); os.environ['PYTHONPATH'] = r'" << p <<"' + os.pathsep + os.environ['PATH']";

				boost::python::exec(python_path_builder.str().c_str(), l_pythonContext.This().global,
				                    l_pythonContext.This().local);

				LOG(INFO) << "Add Python path : " << p.c_str() << std::endl;
			}
		}

		void PythonFilter::initialize_python_paths()
		{
			boost::python::exec("import sys", l_pythonContext.This().global, l_pythonContext.This().local);
			boost::python::exec("import imp", l_pythonContext.This().global, l_pythonContext.This().local);
			boost::python::exec("import os", l_pythonContext.This().global, l_pythonContext.This().local);
			add_python_path(corefilter::getLocalEnv().getValue("python_dll_path"));

			std::string workingDir = GetCurrentWorkingDir();

			add_python_path(workingDir);
			add_python_path(workingDir + "/" + extractDirectoryName(script_path).c_str());

			std::stringstream new_workingDir_command;
			std::string new_workingDir = extractDirectoryName(script_path);
			add_python_path(new_workingDir);

			add_python_path(corefilter::getLocalEnv().getValue("pydata_path"));

			
		}

		void PythonFilter::init_python(const std::string& path)
		{
			if (l_pythonContext.This().main.is_none())
			{
				l_pythonContext.This().main = boost::python::import("__main__");

				std::string moduleName = extractFileName(script_path);

				l_pythonContext.This().global = l_pythonContext.This().main.attr("__dict__");
				l_pythonContext.This().local = l_pythonContext.This().main.attr("__dict__");

				// Initialize the Python Interpreter
				try
				{
					initialize_python_paths();


					l_pythonContext.This().script = boost::python::import(moduleName.c_str());
					l_pythonContext.This().main.attr("__dict__")[moduleName.c_str()] = l_pythonContext
					                                                                         .This().script;


					//Reload module when the python has already loaded the module


					std::stringstream reload_command;
					reload_command << "imp.reload(" << moduleName.c_str() << ");\n";


					boost::python::exec(reload_command.str().c_str(), l_pythonContext.This().global,
					                    l_pythonContext.This().local);


					l_pythonContext.This().global = l_pythonContext.This().script.attr("__dict__");

					//Is there any function name init_process ??
					std::stringstream init_func_name;
					init_func_name << "init_" << function_name;
					
					if (static_cast<boost::python::dict&>(l_pythonContext.This().global).has_key(init_func_name.str()))
					{
						boost::python::object pythonInitCall;
						pythonInitCall = l_pythonContext.This().global[init_func_name.str()];
						boost::python::object result;

						result = pythonInitCall();
					}
					
				}
				catch (boost::python::error_already_set& e)
				{
					if (PyErr_Occurred())
					{
						std::string msg = handle_pyerror();
						LOG(ERROR) << msg << std::endl;
						throw HipeException("Python error : " + msg);
					}
				}
			}
		}

		void PythonFilter::push_result(const boost::python::object& object)
		{
			if (object.is_none())
				return;

			if (boost::python::extract<cv::Mat>(object).check())
			{
				cv::Mat res = boost::python::extract<cv::Mat>(object);

				data::ImageData res_process(res);

				PUSH_DATA(res_process);
			}
			else if (boost::python::extract<std::tuple<int, int>>(object).check())
			{
				std::tuple<int, int> res = boost::python::extract<std::tuple<int, int>>(object);
				std::vector<cv::Point> vec;

				vec.push_back(cv::Point(std::get<0>(res), std::get<1>(res)));

				data::ShapeData res_process;

				res_process.add(vec);

				PUSH_DATA(res_process);
			}
			else if (boost::python::extract<std::tuple<float, float>>(object).check())
			{
				std::tuple<float, float> res = boost::python::extract<std::tuple<float, float>>(object);
				std::vector<cv::Point> vec;

				vec.push_back(cv::Point(std::get<0>(res), std::get<1>(res)));

				data::ShapeData res_process;

				res_process.add(vec);

				PUSH_DATA(res_process);
			}
			else if (boost::python::extract<std::tuple<int, int, int, int>>(object).check())
			{
				std::tuple<int, int, int, int> res = boost::python::extract<std::tuple<int, int, int, int>>(object);
				cv::Rect rect = cv::Rect(std::get<0>(res), std::get<1>(res), std::get<3>(res), std::get<3>(res));

				data::ShapeData res_process;

				res_process.add(rect);

				PUSH_DATA(res_process);
			}
			else if (boost::python::extract<pyShapeData>(object).check())
			{
				data::ShapeData res_process;
				pyShapeData data = boost::python::extract<pyShapeData>(object);

				//std::copy (b.begin(), b.end(), std::back_inserter(a));
				for (cv::Rect rect : data.rects_array())
				{
					res_process.RectsArray().push_back(rect);
				}

				//std::copy (b.begin(), b.end(), std::back_inserter(a));
				for (std::vector<cv::Point2f> quad : data.quadrilatere())
				{
					res_process.QuadrilatereArray().push_back(quad);
				}

				for (cv::Scalar color : data.colors())
				{
					res_process.ColorsArray().push_back(color);
				}

				for (std::string id : data.ids())
				{
					res_process.IdsArray().push_back(id);
				}

				PUSH_DATA(res_process);
			}
			else if (boost::python::extract<pyImageArrayData>(object).check())
			{
				data::ImageArrayData arrays;
				pyImageArrayData pyOutput = boost::python::extract<pyImageArrayData>(object);

				for (int i = 0; i < pyOutput.getArrays().size(); i++)
				{
					data::ImageData img(pyOutput.getArrays()[i]);
					img.setLabel(pyOutput.getLabels()[i]);
					_connexData.push(img);
				}

				

			}
			else if (boost::python::extract<std::string>(object).check())
			{
				std::string res = boost::python::extract<std::string>(object);
				data::ShapeData res_process;
				res_process.IdsArray().push_back(res);
				res_process.RectsArray().push_back(cv::Rect(0, 0, 1, 1));
				PUSH_DATA(res_process);
			}
			else if (boost::python::extract<boost::python::list>(object).check())
			{
				boost::python::list ns = boost::python::extract<boost::python::list>(object);

				data::ShapeData res_process;
				/*for (int i = 0; i < len(ns); ++i)
				{
					
				}
			 
			 res_process.IdsArray().push_back(res);
			 res_process.RectsArray().push_back(cv::Rect(0, 0, 1, 1));
			 PUSH_DATA(res_process);*/
				throw HipeException("The returned list from python isn't yet implemented");
			}
			else
			{
				data::PyObjectData pyObj;
				pyObj.set(object);
				PUSH_DATA(pyObj);

			}
		}

		void PythonFilter::callPythonFunction(boost::python::object& python_function_call,
			std::vector<data::Data>& inputs)
		{
			data::Data in = inputs[0];
			if (in.getType() == data::PYOBJ)
			{
				data::PyObjectData pyObjectInput = static_cast<data::PyObjectData &>(in);

				boost::python::object result; 
			

				result = python_function_call(pyObjectInput.get());


				push_result(result);
			}
			else
			{
				boost::shared_ptr< pyImageData > o(new pyImageData);
				data::ImageData img = inputs[0];
				cv::Mat inMat = img.getMat();

				if (inMat.empty())
				{
					data::ShapeData noneShape;
					PUSH_DATA(noneShape);
				}
				else
				{
					o->assign(inMat);

					boost::python::object result;
					boost::python::ptr(o.get());
					result = python_function_call(boost::python::ptr(o.get()));


					push_result(result);
				}
			}
		}


		HipeStatus PythonFilter::process()
		{
			std::vector<data::Data> input;

			if (_connexData.empty()) return OK;

			while (_connexData.size() != 0)
			{
				data::Data d_ctx = _connexData.pop();
				if (d_ctx.getType() == data::IODataType::PYCTX)
				{
					l_pythonContext = static_cast<const data::PyContextData&>(d_ctx);
				}
				else
				{
					input.push_back(d_ctx);
				}
			}

			PyExternalUser::Use use(*mPyUser);
			try
			{
				std::string l_function_name = "process";
				if (!function_name.empty())
				{
					l_function_name = function_name;
				}


				boost::python::object pythonFunctionCall = l_pythonContext.This().global[l_function_name.c_str()];

				if (!pythonFunctionCall.is_none())
				{
					callPythonFunction(pythonFunctionCall, input);

					
				}
			}
			catch (boost::python::error_already_set& e)
			{
				if (PyErr_Occurred())
				{
					std::string msg = handle_pyerror();
					LOG(ERROR) << msg << std::endl;
					throw HipeException("Python error : " + msg);
				}
				return END_OF_STREAM;
			}

			//Transfer python context to PythonFilter group
			for (auto& pairs : this->getChildrens())
			{
				if (pairs.second->isPython())
				{
					data::DataPort& data_port = static_cast<data::DataPort &>(pairs.second->getConnector().getPort());
					data_port.push(l_pythonContext);
				}
			}


			return OK;
		}

		void PythonFilter::dispose()
		{
			if (_init.exchange(false))
			{
				mPyUser = nullptr;
			}
		}

		void PythonFilter::onLoad(void* interp)
		{
			if (!mPyUser)
			{
				_m_interp = static_cast<PyInterpreterState*>(interp);
			}

		
		}

		void PythonFilter::onStart(void* pyThreadState)
		{
			mPyUser = static_cast<PyExternalUser*>(pyThreadState);
			PyExternalUser::Use use(*mPyUser);
			init_python(extractDirectoryName(script_path));
			
		}
	}
}
