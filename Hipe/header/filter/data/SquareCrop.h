#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <filter/data/IODataType.h>
#include <filter/data/IOData.h>
#include <filter/data/ImageData.h>



namespace filter
{
	namespace data
	{
		class SquareCrop : public IOData<Data, SquareCrop>
		{
		

		protected:
			/**
			 * \brief the list of crop represented by Rectangle and where Rectangle is X,Y the coordiante in the _picture and 
			 * width,height the dimension of the rectangle
			 */
			std::vector<cv::Rect> _squareCrop;

			/**
			 * \brief The image to find the crops
			 */
			ImageData _picture;

			/**
			 * \brief This is a cache list of submatrix cropped and generated by the list of _squareCrop
			 * it will be populate by crops when the number of submatrix differ from the number crops
			 */
			std::vector<cv::Mat> _cropCache;
			
		
			SquareCrop()
			{	

			}

		public:
			using IOData::IOData;

		public:
			SquareCrop(ImageData picture, std::vector<cv::Rect> squareCop) : IOData(IODataType::SQR_CROP)
			{
				Data::registerInstance(new SquareCrop());
				This()._squareCrop = squareCop;
				This()._picture = picture;
				This()._type = SQR_CROP;
			}

			SquareCrop(ImageData picture, std::vector<int> squareCrop) : IOData(IODataType::SQR_CROP)
			{
				Data::registerInstance(new SquareCrop());
				
				This()._picture = picture;
				This()._type = SQR_CROP;

				*(this) << squareCrop; //be carefull Operator<< is a method that undecorate This()
			}

			SquareCrop(const SquareCrop& left) : IOData(left.getType())
			{
				Data::registerInstance(left._This);
				
			}

			std::vector<cv::Rect> getSquareCrop() const
			{
				return This_const()._squareCrop;
			}

			ImageData getPicture() const
			{
				return This_const()._picture;
			}

			virtual SquareCrop& operator=(const SquareCrop& left)
			{
				if (this == &left) return *this;
				if (!left._This) Data::registerInstance(new SquareCrop());
				else
					Data::registerInstance(left._This);
				
				
				return *this;
			}

			IOData& operator<<(const std::vector<int>& left)
			{
				if (left.size() % 4 != 0)
				{
					std::stringstream strbuild;
					strbuild << "Cannot push the list of crop because input " << left.size() << " isn't a modulo of 4 (2 positions X,Y and 2 size width,height)";
					throw HipeException(strbuild.str());
				}

				for (unsigned int index = 0; index < left.size(); index += 4)
				{
					cv::Rect rect(left[index], left[index + 1], left[index + 2], left[index + 3]);
					This()._squareCrop.push_back(rect);
				}

				return *this;
			}

			IOData& operator<<(const std::vector<cv::Rect>& left)
			{
				This()._squareCrop = left;

				return *this;
			}

			IOData& operator<<(const std::vector<cv::Point>& left)
			{
				if (left.size() % 2 != 0)
				{
					std::stringstream strbuild;
					strbuild << "Cannot push the list of crop because input " << left.size() << " isn't a modulo of 2";
					throw HipeException(strbuild.str());
				}

				for (unsigned int index = 0; index < left.size(); index += 2)
				{
					cv::Rect rect(left[index], left[index + 1]);
					This()._squareCrop.push_back(rect);
				}
				
				return *this;
			}

			IOData& operator<<(const ImageData& left)
			{
				if (left.empty()) throw HipeException("No more Image to add in SquareCrop");
				
				This()._picture = left;
			

				return *this;
			}

			IOData& operator<<(const cv::Mat left)
			{
				if (left.empty()) throw HipeException("No more Image to add in SquareCrop");

				This()._picture = ImageData(left);


				return *this;
			}

			/**
			* \brief  The function patterns generate an array of cv::Mat with 
			* all the crop representing a sub-matrix of the pattern image.
			* \return ImageData containing the source to challenge the pattern image
			*/
			std::vector<cv::Mat> & crops(bool forceRefresh = false)
			{
				if (forceRefresh == false && 
					This_const()._cropCache.size() == This_const()._squareCrop.size())
				{
					return This()._cropCache;
				}

				std::vector<cv::Mat> res;

				for (cv::Rect crop : This()._squareCrop)
				{
					res.push_back(This()._picture.getMat()(crop));
				}

				This()._cropCache = res;

				return This()._cropCache;
			}
		};
	}
}
