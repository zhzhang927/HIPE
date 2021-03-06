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

#include <filter/algos/preprocessing/Kmeans.h>
namespace filter
{
	namespace algos
	{
		HipeStatus Kmeans::process()
		{
			while (!_connexData.empty()) // While i've parent data
			{
				data::ImageArrayData images = _connexData.pop();
				if (images.getType() == data::PATTERN)
				{
					throw HipeException("The resize object cant resize PatternData. Please Develop ResizePatterData");
				}

				//Resize all images coming from the same parent
				for (auto &myImage : images.Array())
				{
					myImage = KmeansItt(myImage, clusterCount, attempts);
				}
			}
			return OK;
		}

		//void KmeansIt(cv::Mat img)
		//{
		//	const int MAX_CLUSTERS = 100;
		//	cv::Scalar colorTab[] =
		//	{
		//		cv::Scalar(0, 0, 255),
		//		cv::Scalar(0,255,0),
		//		cv::Scalar(255,100,100),
		//		cv::Scalar(255,0,255),
		//		cv::Scalar(0,255,255)
		//	};
		//	cv::RNG rng(646546);
		//	int k, clusterCount = rng.uniform(2, MAX_CLUSTERS + 1);
		//	int i, sampleCount = rng.uniform(1, 1001);
		//	cv::Mat points(sampleCount, 1, CV_32FC2), labels;
		//	clusterCount = MIN(clusterCount, sampleCount);
		//	cv::Mat centers;
		//	/* generate random sample from multigaussian distribution */
		//	for (k = 0; k < clusterCount; k++)
		//	{
		//		cv::Point center;
		//		center.x = rng.uniform(0, img.cols);
		//		center.y = rng.uniform(0, img.rows);
		//		cv::Mat pointChunk = points.rowRange(k*sampleCount / clusterCount,
		//			k == clusterCount - 1 ? sampleCount :
		//			(k + 1)*sampleCount / clusterCount);
		//		rng.fill(pointChunk, cv::RNG::NORMAL, cv::Scalar(center.x, center.y), cv::Scalar(img.cols*0.05, img.rows*0.05));
		//	}
		//	randShuffle(points, 1, &rng);
		//	kmeans(points, clusterCount, labels,
		//		cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0),
		//		3, cv::KMEANS_PP_CENTERS, centers);
		//	img = cv::Scalar::all(0);
		//	for (i = 0; i < sampleCount; i++)
		//	{
		//		int clusterIdx = labels.at<int>(i);
		//		cv::Point ipt = points.at<cv::Point2f>(i);
		//		circle(img, ipt, 2, colorTab[clusterIdx], cv::FILLED, cv::LINE_AA);
		//	}
		//}

		cv::Mat Kmeans::KmeansItt(cv::Mat myMAt, int clusterCount, int attemps)
		{
			cv::Mat src = myMAt;
			cv::Mat samples(src.rows * src.cols, 3, CV_32F);
			for (int y = 0; y < src.rows; y++)
				for (int x = 0; x < src.cols; x++)
					for (int z = 0; z < 3; z++)
						samples.at<float>(y + x*src.rows, z) = src.at<cv::Vec3b>(y, x)[z];



			cv::Mat labels;
			cv::Mat centers;
			kmeans(samples, clusterCount, labels, cv::TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 10000, 0.0001), attempts, cv::KMEANS_PP_CENTERS, centers);


			cv::Mat new_image(src.size(), src.type());
			for (int y = 0; y < src.rows; y++)
				for (int x = 0; x < src.cols; x++)
				{
					int cluster_idx = labels.at<int>(y + x*src.rows, 0);
					new_image.at<cv::Vec3b>(y, x)[0] = centers.at<float>(cluster_idx, 0);
					new_image.at<cv::Vec3b>(y, x)[1] = centers.at<float>(cluster_idx, 1);
					new_image.at<cv::Vec3b>(y, x)[2] = centers.at<float>(cluster_idx, 2);
				}
			return new_image;
		}
	}
}




