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

#pragma once
#include <corefilter/tools/RegisterClass.h>
#include <core/HipeException.h>
#include <corefilter/IFilter.h>
#include <core/HipeStatus.h>
#include <data/ImageArrayData.h>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"




namespace filter
{
	namespace algos
	{
		/**
		 * \var Kmeans::attempts
		 * The number of times the kmeans algorithm is executed using different initial labellings. The algorithm will return the labels that yield the best compactness. \see cv::kmeans
		 *
		 * \var Kmeans::clusterCount
		 * The number of clusters (colors) the output image should have.
		 */

		/**
		 * \brief K-means image segmentation. The filter will find color clusters to compress image and reduce the number of used colors.
		 */
		class Kmeans : public filter::IFilter
		{
			CONNECTOR(data::ImageArrayData, data::ImageArrayData);
			REGISTER(Kmeans, ()), _connexData(data::INOUT)
			{

			}

			REGISTER_P(int, attempts);
			REGISTER_P(int, clusterCount);

			virtual std::string resultAsString() { return std::string("TODO"); };
			cv::Mat KmeansItt(cv::Mat myMAt, int clusterCount, int attemps);
		public:
			HipeStatus process();

		};

		ADD_CLASS(Kmeans, clusterCount, attempts);
	}
}
