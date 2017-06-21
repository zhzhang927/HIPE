#pragma once
#include <iostream>

#include <filter/tools/RegisterClass.h>
#include <core/HipeException.h>
#include <filter/IFilter.h>
#include <core/HipeStatus.h>


#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>

#include <dlib/opencv.h>
#include <opencv2/highgui/highgui.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>

namespace filter
{
	namespace algos
	{
		class FaceDetection : public filter::IFilter
		{
			int count_frame;
			dlib::frontal_face_detector detector;
			std::vector<dlib::rectangle> dets;
			dlib::shape_predictor pose_model;

			std::atomic<bool> isStart;
			core::queue::ConcurrentQueue<data::ImageData> imagesStack;
			core::queue::ConcurrentQueue<data::SquareCrop> crops;
			data::SquareCrop tosend;

			boost::thread *thr_server;
			//data::ConnexData<data::ImageArrayData, data::ImageArrayData> _connexData;
			CONNECTOR(data::ImageArrayData, data::SquareCrop);

			REGISTER(FaceDetection, ()), _connexData(data::INDATA)
			{
				count_frame = 0;
				skip_frame = 4;
				detector = dlib::get_frontal_face_detector();
				isStart = true;
				thr_server = nullptr;

				
			
				dlib::deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;

				startDetectFace();
			}

			REGISTER_P(int, skip_frame);


			virtual std::string resultAsString() { return std::string("TODO"); };
			
		public:
			void startDetectFace()
			{
				FaceDetection* This = this;
				thr_server = new boost::thread([This]
				{
					while (This->isStart)
					{
						data::ImageData image;
						if (!This->imagesStack.trypop_until(image, 300))
							continue;

						This->detectFaces(image);
						std::vector<cv::Rect> rects;
						
						for (dlib::rectangle & rect : This->dets)
						{
							cv::Rect cvRect;
							cvRect.x = rect.left();
							cvRect.y = rect.top();
							cvRect.height = rect.bottom() - rect.top();
							cvRect.width = rect.right() - rect.left();
							rects.push_back(cvRect);
						}
						data::SquareCrop crop(image, rects);

						if (This->crops.size() == 0)
							This->crops.push(crop);


					}
				});
			}
			
			void detectFaces(const data::ImageData & image)
			{
				try
				{
					dlib::array2d<unsigned char> img;
					dlib::cv_image<dlib::bgr_pixel> cimg(image.getMat());
					dlib::assign_image(img, cimg);


					// Make the image bigger by a factor of two.  This is useful since
					// the face detector looks for faces that are about 80 by 80 pixels
					// or larger.  Therefore, if you want to find faces that are smaller
					// than that then you need to upsample the image as we do here by
					// calling pyramid_up().  So this will allow it to detect faces that
					// are at least 40 by 40 pixels in size.  We could call pyramid_up()
					// again to find even smaller faces, but note that every time we
					// upsample the image we make the detector run slower since it must
					// process a larger image.
					//dlib::pyramid_up(img); // Resizing is done outside the control of face detecttion in Hipe
					// please refer to Resize class to any resizing

					// Now tell the face detector to give us a list of bounding boxes
					// around all the faces it can find in the image.
					dets = detector(img);

					std::cout << "Number of faces detected: " << dets.size() << std::endl;
					//// Now we show the image on the screen and the face detections as
					//// red overlay boxes.
					//win.clear_overlay();
					//win.set_image(img);
					//win.add_overlay(dets, dlib::rgb_pixel(255,0,0));


					//std::cout << "Hit enter to process the next image..." << std::endl;
					//std::cin.get();
				}
				catch (exception& e)
				{
					cout << "\nexception thrown!" << endl;
					cout << e.what() << endl;
				}
			}

			
			HipeStatus process()
			{
				cv::Mat im;
				cv::Mat im_small, im_display;
 
				
				
				std::vector<dlib::rectangle> faces;

				
				{
					data::ImageArrayData images = _connexData.pop();
					if (images.getType() == data::PATTERN)
					{
						throw HipeException("The resize object cant resize PatternData. Please Develop FaceDetectionPatterData");
					}
					
					//FaceDetection all images coming from the same parent
					for (auto &myImage : images.Array()) 
					{
						if (count_frame % skip_frame == 0)
						{
							imagesStack.push(myImage);
						}
						count_frame++;
					}
					//TODO manage list of SquareCrop. For now consider there only one SquareCrop
					data::SquareCrop popSquareCrop;
					if (crops.trypop_until(popSquareCrop, 30)) // wait 30ms no more
					{
						_connexData.push(popSquareCrop);
						tosend = popSquareCrop;
					}
					else {

						_connexData.push(tosend);
					}
				}
				return OK;
			}


			virtual void dispose()
			{
				isStart = false;

				if (thr_server != nullptr) {
					thr_server->join();
					delete thr_server;
					thr_server = nullptr;
				}
			}
		};

		ADD_CLASS(FaceDetection, skip_frame);
	}
}
