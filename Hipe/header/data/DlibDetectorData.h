#pragma once
#include <data/IOData.h>
#include <data/Data.h>
#include <data/IODataType.h>

#include <boost/thread/shared_mutex.hpp>
#include <dlib/image_processing.h>
#include <data/HogTrainer/HogTrainer.h>

namespace data
{
	/**
	* \brief DlibDetectorData is the data class used to handle dlib detectors. Uses Dlib.
	*/
	class DATA_EXPORT DlibDetectorData : public data::IOData<data::Data, DlibDetectorData>
	{
		typedef dlib::object_detector<::data::hog_trainer::image_scanner_type> detector_type;

	private:
		//std::shared_ptr<const std::vector<detector_type> > _detectors = nullptr;
		//std::shared_ptr<boost::shared_mutex> _mutex = nullptr;

		std::vector<detector_type> _detectors;

	protected:

		DlibDetectorData(IOData::_Protection priv) : IOData(data::IODataType::DLIBDTCT)
		{

		}

		DlibDetectorData(data::IODataType type) :IOData(type)
		{

		}

	public:
		using IOData::IOData;


		/**
		* \brief DlibDetectorData default constructor, the internal IODataType data type will be "DLIBDTCT"
		*/
		DlibDetectorData() : IOData(::data::IODataType::DLIBDTCT)
		{
			data::Data::registerInstance(new DlibDetectorData(IOData::_Protection()));
			This()._type = data::IODataType::DLIBDTCT;
			_type = data::IODataType::DLIBDTCT;
		}


		/**
		* \brief DlibDetectorData copy constructor
		* \param right The DlibDetectorData to copy data from
		*/
		DlibDetectorData(const data::DlibDetectorData &right) : IOData(right._type)
		{
			data::Data::registerInstance(right._This);
			_type = right.This_const()._type;
			_decorate = right._decorate;
			_detectors = right._detectors;
			//_mutex = right._mutex;
		}

		//DlibDetectorData(const std::vector<detector_type> & detectors, std::shared_ptr<boost::shared_mutex> mutex) : IOData(data::IODataType::DLIBDTCT), _detectors(&detectors)
		DlibDetectorData(const std::vector<detector_type> & detectors) : IOData(data::IODataType::DLIBDTCT)
		{
			data::Data::registerInstance(new DlibDetectorData(IOData::_Protection()));
			//Data::registerInstance(new DlibDetectorData(_detectors, mutex));
			This()._type = data::IODataType::DLIBDTCT;
			This()._detectors = detectors;
			_type = data::IODataType::DLIBDTCT;
			_detectors = detectors;
		}

		virtual ~DlibDetectorData()
		{
			IOData::release();
		}



		/**
		* \brief Accessor (const version) to the detectors container
		* \return Returns a const reference to the detectors container.
		*/
		const std::vector<detector_type>& detectors_const() const;

		/**
		 * \brief Accessor to the object mutex
		 * \return Returns a shared_tr to the object's mutex.
		 */
		std::shared_ptr<boost::shared_mutex> mutex_ptr();

		/**
		* \brief DlibDetectorData assignment operator.
		* \param left The DlibDetectorData oject to get the data from.
		* \return A reference to the object.
		*/
		DlibDetectorData& operator=(const DlibDetectorData& left);
	};
}