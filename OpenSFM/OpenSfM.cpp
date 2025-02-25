#include "OpenSfM.h"
#include "GeometryFunctions.h"

using namespace std;
using namespace cv;

int OpenSfM::run(){
	// dir should be dir of images, while for now, we just load two images
	// 
	cout<<"start to run OpenSfM........."<<endl<<endl;

	Mat imgA = this->images[0];
	Mat imgB = this->images[1];
	if( !imgA.data || !imgB.data){
		cerr<<"no data!\n"<<endl;
	}

	if(DEBUG) {
			Mat H; hconcat(imgA,imgB,H);
			imshow("initial two view",H);
			waitKey(0);
	}


	last_frame* last_f = initalTwoViewRecon(imgA, imgB);

	return 0;
}



last_frame* OpenSfM::initalTwoViewRecon(cv::Mat& imA, cv::Mat& imB){
	
	/*  STEP 1
		Detect SIFT Feature
	 */
	SiftFeatureDetector detector;
	vector<KeyPoint> keypointA, keypointB;
	detector.detect(imA,keypointA);
	detector.detect(imB,keypointB);

	if(DEBUG) {
		/* code */
		 Mat img_keypoints_1; Mat img_keypoints_2;

 		 drawKeypoints( imA, keypointA, img_keypoints_1, Scalar::all(-1), DrawMatchesFlags::DEFAULT );
  		 drawKeypoints( imB, keypointB, img_keypoints_2, Scalar::all(-1), DrawMatchesFlags::DEFAULT );

 		 //-- Show detected (drawn) keypoints
 		 imshow("Keypoints 1", img_keypoints_1 );
 		 imshow("Keypoints 2", img_keypoints_2 );

 		 waitKey(0);
	}

	/*  STEP 2
		Extract SIFT desc
	 */
	
	SiftDescriptorExtractor extractor;

	Mat descA, descB;
	extractor.compute(imA,keypointA,descA);
	extractor.compute(imB,keypointB,descB);


	/*-- Step 3: 
		Matching descriptor vectors using FLANN matcher
	*/

	 FlannBasedMatcher matcher;
	 vector< DMatch > matches;
	 matcher.match( descA, descB, matches );

	 double max_dist = 0; double min_dist = 100;

  	/* Setp 4 Good match
  	 Quick calculation of max and min distances between keypoints
  	 */
	  for( int i = 0; i < descA.rows; i++ ){
	  	 double dist = matches[i].distance;
	    if( dist < min_dist ) min_dist = dist;
	    if( dist > max_dist ) max_dist = dist;
	  }

	 vector< DMatch > good_matches, test_matches;
	 vector< KeyPoint >P1, P2;
	 vector< Point2f > P1f, P2f;

	 int cnt = 0;
	  for( int i = 0; i < descA.rows; i++ ){
	   if( matches[i].distance <= max(this->min_dist*min_dist, 0.02) ){
	   	 good_matches.push_back( matches[i]);
	   	 test_matches.push_back(DMatch(cnt,cnt,1.0)); cnt++;
	   	 P1.push_back(keypointA[matches[i].queryIdx]);
	   	 P2.push_back(keypointB[matches[i].trainIdx]);
	   	 P1f.push_back(keypointA[matches[i].queryIdx].pt);
	   	 P2f.push_back(keypointB[matches[i].trainIdx].pt);
	   	  // cout<<"P1: "<<P1.back().pt<<endl;
	   	   // cout<<"P1f: "<<P1f.back()<<endl;
	   	   // cout<<"P2f: "<<P2f.back()<<endl;
	   	}
	  }
	  matches.clear();


	  if(DEBUG) {
	  	/* code */
	  	printf("-- Max dist : %f \n", max_dist );
  		printf("-- Min dist : %f \n", min_dist );
  		Mat img_matches;
  		drawMatches( imA, P1, imB, P2,
               test_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
               vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

 	 	//-- Show detected matches
  		imshow( "Good Matches", img_matches );

  		cout<<"size of desc A: "<<descA.rows<<" | "<<descA.cols<<endl;
  		cout<<"size of desc B: "<<descB.rows<<" | "<<descB.cols<<endl;
  		cout<<"size of good matches: "<<good_matches.size()<<endl;

  		waitKey(0);

	  }
	  //P1.clear();
	  //P2.clear();
	  keypointA.clear();
	  keypointB.clear();



	  /* STEP 5  get F matrix
	  	and check Epolier line
	   */
	  Mat mask;
	  Mat fundamental_matrix = findFundamentalMat(P1f, P2f, FM_RANSAC, 0.1, 0.99, mask);

	  cout<<"\nwhat???!"<<fundamental_matrix<<endl;
	  if(DEBUG) {
	  	/* code */
	  	Mat Epilines;
	  	computeCorrespondEpilines(Mat(P1f),1,fundamental_matrix,Epilines);
	  	cout<<"size of Epilines: "<<Epilines.rows<<" | "<<Epilines.cols<<endl;
	  	cout<<"depth of Epilines: "<<Epilines.depth()<<endl;

	  	//float top_horizontal[3] =    {0, 1, 0};
	  	 Point3f top_horizontal = Point3f(0,1,0);
		 Point3f left_vertical  =   Point3f(1, 0, 0); 
		 Point3f bottom_horizontal = Point3f(0, 1, -imA.rows); 
		 Point3f right_vertical =    Point3f(1, 0, -imA.cols); 

	  	Mat Epilines_show = imB;
	 	for(int i = 0; i < Epilines.rows; i++){
	 		Point2f A;
	 		Point2f B;

	 		Point3f Eline = Point3f(Epilines.at<float>(i,0),Epilines.at<float>(i,1),Epilines.at<float>(i,2));
	 		Point3f candidate_1 = top_horizontal.cross(Eline);
	 		Point2f candidate_1_cord = Point2f(candidate_1.x/candidate_1.z, candidate_1.y/candidate_1.z);
	 		if(candidate_1_cord.x >= 0 && candidate_1_cord.x <= imA.cols) {
	 			/* code */
	 			A = candidate_1_cord;
	 		}
	 		Point3f candidate_2 = left_vertical.cross(Eline);
	 		Point2f candidate_2_cord = Point2f(candidate_2.x/candidate_2.z, candidate_2.y/candidate_2.z);
	 		if(candidate_2_cord.y >= 0 && candidate_2_cord.y <= imA.cols) {
	 			/* code */
	 			A = candidate_2_cord;
	 		}
	 		Point3f candidate_3 = bottom_horizontal.cross(Eline);
	 		Point2f candidate_3_cord = Point2f(candidate_3.x/candidate_3.z, candidate_3.y/candidate_3.z);
	 		if(candidate_3_cord.x >= 0 && candidate_3_cord.x <= imA.cols) {
	 			/* code */
	 			B = candidate_3_cord;
	 		}
	 		Point3f candidate_4 = right_vertical.cross(Eline);
	 		Point2f candidate_4_cord = Point2f(candidate_4.x/candidate_4.z, candidate_4.y/candidate_4.z);
	 		if(candidate_4_cord.y >= 0 && candidate_4_cord.y <= imA.cols) {
	 			/* code */
	 			B = candidate_4_cord;
	 		}

	 		line(Epilines_show,A,B,Scalar(0,255,0));
	 		// cout<<"point: "<<x<<"  "<<y<<endl;
	 	}
	 	Mat Epilines_show_pts;
	 	drawKeypoints( Epilines_show, P2, Epilines_show_pts, Scalar::all(-1), DrawMatchesFlags::DEFAULT );
	 	imshow("Epilines_show",Epilines_show_pts);
	 	waitKey(0);
	  }

	  /* STEP 6  
	  	create tables
	   */
	  int NUM = 0;
	  vector< Point2f > P1_inliers, P2_inliers;
	  for(unsigned i = 0; i < mask.rows; ++i) {
	  	/* extract inliears */
	  	if(mask.at<char>(i,0) == 1){ NUM++;
	  		P1_inliers.push_back(P1f[i]);
	  		P2_inliers.push_back(P2f[i]);
	  	}
	  }


	  //handle featureTable;
	  int num_of_feature = P1f.size();
	  this->featureTable = new Mat(num_of_feature,128+3+3,CV_32FC1);
	  this->featureCell = new vector<arma::umat*>();
	  this->cameraPose = new vector<arma::fmat*>();
	  this->camProjTable = new vector<arma::fmat*>();
	  this->Z_i = new vector<unsigned>(num_of_feature);
	  iota((this->Z_i)->begin(),(this->Z_i)->end(),0);
	  this->Z_j = new vector<unsigned>(num_of_feature);
	  iota((this->Z_j)->begin(),(this->Z_j)->end(),0);
	  this->Z_v = new vector<unsigned>(num_of_feature,1);



	  if(DEBUG){
			cout<<"depth of mask: "<<mask.depth()<<endl;
			cout<<"# of inliners: "<<NUM<<endl;
			cout<<"# of all_features: "<<num_of_feature<<endl;
		}

	  for(unsigned i = 0; i < num_of_feature; ++i) {
			/* insert tabel */
	  		//1.copy desc of camB to the table for future match
	  		descB.row(good_matches[i].trainIdx).copyTo(((this->featureTable)->row(i)).colRange(0,128));
	  		//if(DEBUG) cout<<(this->featureTable)->row(i).colRange(0,2)<<" | "<<descB.row(good_matches[i].trainIdx).colRange(0,2)<<endl;
	  		
	  		//2.insert correspounding 2D points to feature cell
	  		arma::umat* tmp_feature_per_cell = new arma::umat(2,2);
	  		*tmp_feature_per_cell << P1f[i].x << P1f[i].y << arma::endr << P2f[i].x << P2f[i].y << arma::endr;
	  		featureCell->push_back(tmp_feature_per_cell);

		}
		//3. init Z table;
		//done when initilized

		//4. init camProjTable
		
		arma::fmat* tmp_camProjTable = new arma::fmat(3,4);
		*tmp_camProjTable  << 1 << 0 << 0 << 0 << arma::endr
						   << 0 << 1 << 0 << 0 << arma::endr
						   << 0 << 0 << 1 << 0;
		*tmp_camProjTable = (this->intrinsc_K)*(*tmp_camProjTable);
		this->camProjTable->push_back(tmp_camProjTable);
		//this->camProjTable->push_back(tmp_camProjTable);	

		// solve projCam for the camB
		
		Mat K = Mat(3,3,CV_32FC1,(this->intrinsc_K).memptr());
		K.convertTo(K,CV_64FC1);
		K = K.t();
		cout<<"fuck222\n";
		if(DEBUG) cout<<" intrinsc_K : "<<K<<endl;
		if(DEBUG) cout<<" FFF : "<<fundamental_matrix<<endl;

		//Mat E = K.t()*fundamental_matrix*K;
		vector<arma::fmat> Projs_camB;
		AllPossiblePFromF(fundamental_matrix, K, Projs_camB);

		for(unsigned i = 0; i < Projs_camB.size(); ++i) {
			/* code */
			Projs_camB[i] = (this->intrinsc_K)*Projs_camB[i];
		}
		
		Mat camP_A( 4, 3, CV_32FC1, (*camProjTable)[0]->memptr() ); camP_A = camP_A.t();
		vector<Mat> point4D(4);
			K.convertTo(K,CV_32FC1);
			cout<<"Projs_camB size:"<<Projs_camB.size()<<endl;
			int max_correct_pts = 0; int best_index = -1;
		for(unsigned i = 0; i < Projs_camB.size(); ++i){
			/* code */
			int counter_correct_pts = 0;
			Mat camP_B( 4, 3, CV_32FC1, Projs_camB[i].memptr() ); camP_B = camP_B.t();
			triangulatePoints(camP_A,camP_B,P1_inliers,P2_inliers,point4D[i]);
			cout<<"size of point4D:"<<endl<<point4D[i].cols<<endl<<point4D[i].rows<<endl;

			convertPointsFromHomogeneous(point4D[i].t(),point4D[i]);
						cout<<"\n"<<point4D[i]<<"\n";

			Mat tmo(point4D[i].rows,3,CV_32FC1);
			for(unsigned x = 0; x < point4D[i].rows; ++x) {
				/* fix openCV bug */
				tmo.row(x) = point4D[i].row(x);
				if(point4D[i].at<float>(x,2) > 0) counter_correct_pts++;
			}
			Mat one = Mat::ones(tmo.rows,1,CV_32FC1);
			hconcat(tmo,one,point4D[i]);
			Mat point4D_in_camB = K.inv()*camP_B*point4D[i].t();
						//cout<<"\n"<<point4D_in_camB<<"\n";

			for(unsigned x = 0; x < point4D_in_camB.cols; ++x) {
				/* code */
				if(point4D_in_camB.at<float>(2,x) > 0) counter_correct_pts++;
			}
			cout<<"counter_correct_pts: "<<counter_correct_pts<<" / "<< point4D[i].rows*2<<endl;
			if(counter_correct_pts > max_correct_pts){
				best_index = i;
				max_correct_pts = counter_correct_pts;
			}
		}
		// best_index = 2;
		cout<<"best index: "<<best_index<<endl;  
		arma::fmat *tmp_camProjTable_B = new arma::fmat(3,4); *tmp_camProjTable_B = Projs_camB[best_index];
		this->camProjTable->push_back(tmp_camProjTable_B);
	  
	  	//handle poseTable
		Mat camP_B( 4, 3, CV_32FC1, tmp_camProjTable_B->memptr() ); camP_B = camP_B.t();
		Mat Rot_B, camProj, trans_B;
		decomposeProjectionMatrix(camP_B,camProj,Rot_B,trans_B);
		 cout<<"depth of trans_B: "<<trans_B.size()<<endl;
		 cout<<"computed intrinsc_K: "<<camProj<<endl;
		// convertPointsFromHomogeneous(trans_B,trans_B);
		trans_B = trans_B/trans_B.at<float>(0,3);
		if(DEBUG){
			cout<<"Rot_B: "<<Rot_B<<endl
				<<"trans_B: "<<trans_B<<endl;
		}
	  	arma::fmat pose_B_rot( reinterpret_cast<float*>(Rot_B.data), Rot_B.rows, Rot_B.cols );
	  	arma::fmat pose_B_trans( reinterpret_cast<float*>(trans_B.data), trans_B.rows, trans_B.cols );
	  	arma::fmat tmp_pose_B = join_rows(pose_B_rot.t(),pose_B_trans.rows(0,2));
	  	// cout<<tmp_pose_B.n_rows<<tmp_pose_B.n_cols<<endl;
	  	// cout<<tmp_pose_B<<endl;
	  	
	  	 arma::fmat * pose_B = new arma::fmat(3,4);
	  	 *pose_B = tmp_pose_B.t();
	  	
	  	 this->cameraPose->push_back(pose_B);

	  	// cout<<"camPose B: "<<*pose_B<<endl;

	 // mat arma_mat( reinterpret_cast<double*>opencv_mat.data, opencv_mat.rows, opencv_mat.cols )



	return new last_frame;


	}


