#include "header.hpp"
int xresol = 1;
int yresol = 1;

void CallBackFunc(int event, int x, int y, int flags, void *params)
{
    if (event == EVENT_LBUTTONDOWN)
    {
        vector<Point2f> *points = (vector<Point2f> *)params;
        if (points->size() < 4)
        {
            points->push_back(Point2f(x, y));
        }
    }
}

int get_quad(const Point &p1)
{
    if (p1.x > 0 && p1.y < 0)
        return 4;
    else if (p1.x < 0 && p1.y < 0)
        return 1;
    else if (p1.x < 0 && p1.y > 0)
        return 2;
    return 3;
}
bool compare_points(const Point &a, const Point &b)
{
    return (get_quad(a) < get_quad(b));
}
vector<Point2f> sort_points(vector<Point2f> points)
{
    int Mx = (points[0].x + points[1].x + points[2].x + points[3].x) / 4;
    int My = (points[0].y + points[1].y + points[2].y + points[3].y) / 4;
    for (int a = 0; a < 4; a++)
    {
        points[a].x -= Mx;
        points[a].y -= My;
    }
    std::sort(points.begin(), points.end(), compare_points);
    for (int a = 0; a < 4; a++)
    {
        points[a].x += Mx;
        points[a].y += My;
        cout << "(" << points[a].x << ", " << points[a].y << ")" << endl;
    }
    return points;
}
int display_whiteratio_dynamic(Mat &gry, Mat &frame, vector<int> &dynamic_y, int pastValue)
{
    if (pastValue == -1)
    {
        vector<Point> white_pixels_dyn;
        cv::findNonZero(gry, white_pixels_dyn);
        int white_ratio_dyn = ((white_pixels_dyn.size()) * 1000.0) / (gry.cols * gry.rows);
        stringstream sst;
        sst << white_ratio_dyn;
        dynamic_y.push_back(white_ratio_dyn);
        white_pixels_dyn.clear();
        rectangle(frame, cv::Point(10, 30), cv::Point(100, 48), cv::Scalar(255, 255, 255), -1); //Display white ratio on top left corner
        string frameNumberString = sst.str();
        putText(frame, frameNumberString.c_str(), cv::Point(45, 43),
                FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
        return white_ratio_dyn;
    }
    else
    {
        dynamic_y.push_back(pastValue);
        return pastValue;
    }
}
int display_whiteratio_queue(Mat &fgMask, Mat &frame, vector<int> &queue_y, int pastValue)
{
    if (pastValue == -1)
    {
        rectangle(frame, cv::Point(10, 2), cv::Point(100, 20), cv::Scalar(255, 255, 255), -1); //Display white ratio on top left corner
        stringstream ss;
        //ss << capture.get(CAP_PROP_POS_FRAMES); //0-based index of the frame to be decoded/captured next.
        vector<Point> white_pixels;
        cv::findNonZero(fgMask, white_pixels);
        //ss << capture.get(CAP_PROP_POS_FRAMES);
        int white_ratio = ((white_pixels.size()) * 1000.0) / (fgMask.cols * fgMask.rows);
        ss << white_ratio;
        queue_y.push_back(white_ratio);
        white_pixels.clear();
        string frameNumberString = ss.str();
        putText(frame, frameNumberString.c_str(), cv::Point(45, 15),
                FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
        return white_ratio;
    }
    else
    {
        queue_y.push_back(pastValue);
        return pastValue;
    }
}
void write_out_queue(vector<int> queue_y)
{
    freopen("../outputs/static.out", "w", stdout);
    for (int i = 0; i < queue_y.size(); i++)
    {
        if (i == queue_y.size() - 1)
        {
            cout << queue_y[i] / 1000.0;
            break;
        }

        cout << queue_y[i] / 1000.0 << ",";
    }
}
void write_out_dynamic(vector<int> dynamic_y)
{
    freopen("../outputs/dynamic.out", "w", stdout);
    for (int i = 0; i < dynamic_y.size(); i++)
    {
        if (i == dynamic_y.size() - 1)
        {
            cout << dynamic_y[i] / 1000.0;
            break;
        }
        cout << dynamic_y[i] / 1000.0 << ",";
    }
}
Mat evaluate_dense_opticalflow(Mat &next, Mat &prvs, Mat frame)
{
    cvtColor(frame, next, COLOR_BGR2GRAY);
    Mat flow(prvs.size(), CV_32FC2);
    calcOpticalFlowFarneback(prvs, next, flow, 0.5, 3, 15, 3, 7, 1.5, 0);
    // visualization
    Mat flow_parts[2];
    split(flow, flow_parts);

    //flow parts[0] has x coordinates of pixel displacement vectors
    //flow parts[1] has y coordinates of pixel displacement vectors
    Mat magnitude, angle, magn_norm, gr_bt, temp;

    //magnitude and angle calculation using x and y coordinate vectors
    //angle in degrees is set to true
    cartToPolar(flow_parts[0], flow_parts[1], magnitude, angle, true);

    //normalize the magnitude Matrix and output into magn_norm Matrix
    // Min = 0 and Max = 1
    normalize(magnitude, magn_norm, 0.0f, 1.0f, NORM_MINMAX);
    //threshold(temp, magn_norm, 0.4f, 1.0f, THRESH_BINARY);
    //threshold(gr_bt, magn_norm,0.8,1.0, THRESH_BINARY);
    angle *= ((1.f / 360.f) * (180.f / 255.f));
    //build hsv image
    Mat _hsv[3], hsv, hsv8, bgr, gry;
    _hsv[0] = angle;
    _hsv[1] = Mat::ones(angle.size(), CV_32F);
    _hsv[2] = magn_norm;
    merge(_hsv, 3, hsv);
    hsv.convertTo(hsv8, CV_8U, 255.0);
    cvtColor(hsv8, bgr, COLOR_HSV2BGR);
    cvtColor(bgr, gr_bt, COLOR_BGR2GRAY);
    threshold(gr_bt, gry, 15, 255, THRESH_BINARY);
    return gry;
}
Mat evaluate_lucas_kanade_opticalflow(Mat &frame, vector<Point2f> &p0, vector<Point2f> &p1, vector<Point2f> &good_new, Mat &mask, Mat &old_gray, Mat &frame_gray, vector<Scalar> colors, vector<int> &sparse)
{
    //Mat frame_gray;
    cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
    vector<uchar> status;
    vector<float> err;
    TermCriteria criteria = TermCriteria((TermCriteria::COUNT) + (TermCriteria::EPS), 10, 0.03);
    calcOpticalFlowPyrLK(old_gray, frame_gray, p0, p1, status, err, Size(15, 15), 2, criteria);
    //vector<Point2f> good_new;
    //We pass the previous frame, previous points and next frame.
    //It returns next points along with some status numbers which
    //has a value of 1 if next point is found, else zero
    double euclid = 0.0;
    for (uint i = 0; i < p0.size(); i++)
    {
        // Select good points
        if (status[i] == 1)
        {
            //status = 1 implies the the point is found
            good_new.push_back(p1[i]);
            // draw the tracks
            //cout <<< p1[i].x << " " << p1[i].y << " " << p0[i].x << " " << p0[i].y << endl;
            euclid += sqrt((p1[i].x - p0[i].x) * (p1[i].x - p0[i].x) + (p1[i].y - p0[i].y) * (p1[i].y - p0[i].y));
            line(mask, p1[i], p0[i], colors[i], 2);
            circle(frame, p1[i], 5, colors[i], -1);
        }
    }
    sparse.push_back(euclid / 10);
    Mat img;
    add(frame, mask, img);
    return img;
}
void changeResol(Mat &img1)
{
    resize(img1, img1, Size(yresol, xresol));
}
int main(int argc, char const *argv[])
{
    string image1_path = samples::findFile("../assets/empty.jpg");
    Mat img1 = imread(image1_path, IMREAD_GRAYSCALE);
    xresol = atoi(argv[1]);
    yresol = atoi(argv[2]);
    changeResol(img1);
    vector<Point2f> points;
    // UNCOMMENT FROM HERE AFTER SCRIPT IS OVER
    namedWindow("Display window", WINDOW_NORMAL);
    resizeWindow("Display window", 1000, 1000);
    imshow("Display window", img1);
    cout << "Selected points are: " << endl;
    while (points.size() < 4)
    {
        setMouseCallback("Display window", CallBackFunc, &points);
        waitKey(500);
    }
    destroyWindow("Display window");
    points = sort_points(points);
    // points.push_back(Point2f(948, 270));
    // points.push_back(Point2f(205, 1062));
    // points.push_back(Point2f(1551, 1064));
    // points.push_back(Point2f(1296, 249));
    vector<Point2f> pts_dst;

    pts_dst.push_back(Point2f(points[0].x, points[0].y));
    pts_dst.push_back(Point2f(points[0].x, points[1].y));
    pts_dst.push_back(Point2f(points[3].x, points[1].y));
    pts_dst.push_back(Point2f(points[3].x, points[0].y));

    Mat H = findHomography(points, pts_dst);
    Rect crop_region(points[0].x, points[0].y, points[3].x - points[0].x, points[1].y - points[0].y);

    string vid_path = "../assets/trafficvideo.mp4";

    VideoCapture capture(samples::findFile(vid_path));
    int noOfFrames = capture.get(CAP_PROP_FRAME_COUNT);

    if (!capture.isOpened())
    {
        cout << "Could not open file: " << vid_path << endl;
        return 0;
    }
    Ptr<BackgroundSubtractor> obj_back;                       //create Background Subtractor object
    obj_back = createBackgroundSubtractorMOG2(1, 350, false); // (History, threshold, detech_shadows = false)
    //obj_back = createBackgroundSubtractorKNN();

    int framec = 0;

    Mat frame, fgMask, prvs, frame1, old_gray;
    vector<int> queue_y;
    vector<int> dynamic_y;
    vector<int> sparse_y;
    vector<Point2f> p0, p1;

    capture >> frame1;
    changeResol(frame1);
    warpPerspective(frame1, frame1, H, frame.size());
    frame1 = frame1(crop_region);
    cvtColor(frame1, prvs, COLOR_BGR2GRAY);
    warpPerspective(img1, img1, H, img1.size());
    // frame1 = img1;
    frame = img1(crop_region);
    //obj_back->apply(frame, fgMask, 1);
    double fps = capture.get(CAP_PROP_FPS);
    double timeOfVid = noOfFrames / fps;
    int processf = 20;
    int qPastValue = 0;
    int dPastValue = 0;
    //==================
    // Create some random colors
    vector<Scalar> colors;
    RNG rng;
    for (int i = 0; i < 1000; i++)
    {
        int r = rng.uniform(0, 256);
        int g = rng.uniform(0, 256);
        int b = rng.uniform(0, 256);
        colors.push_back(Scalar(r, g, b));
    }
    //old_frame = frame1
    cvtColor(frame1, old_gray, COLOR_BGR2GRAY);
    goodFeaturesToTrack(old_gray, p0, 1000, 0.1, 7, Mat(), 7, false, 0.04);
    // Create a mask image for drawing purposes
    //p0 -> contains corners of frame old_gray
    //100 -> Number of corners
    //0.3 -> corners with less than 0.3*best_corner_quality are rejected
    //7 -> Minimum possible Euclidean distance between the returned corners
    //Mat mask = Mat::zeros(frame1.size(), frame1.type());
    //======================
    auto start = high_resolution_clock::now();
    while (true)
    {
        capture.read(frame);

        if (frame.empty())
            break;
        changeResol(frame);
        // cout << framec << endl;
        if (framec % processf != 0)
        {
            framec++;
            queue_y.push_back(qPastValue);
            dynamic_y.push_back(dPastValue);
            continue;
        }
        framec++;
        if (framec == 999999)
        {
            break;
        }
        warpPerspective(frame, frame, H, frame.size());
        frame = frame(crop_region);
        Mat frame_new = frame.clone();
        // Display the resulting frame
        // imshow("Frame", frame);
        fgMask = img1;
        obj_back->apply(frame, fgMask, 0); //Learning rate set to 0

        //==================================================================================
        //Display white ratio in white box on top left corner for masked frames
        qPastValue = display_whiteratio_queue(fgMask, frame, queue_y, -1);
        //==================================================================================
        //vector<Point2f> good_new;
        //Mat frame_gray;
        //Mat mask = Mat::zeros(frame1.size(), frame1.type());
        //Mat img_lc = evaluate_lucas_kanade_opticalflow(frame_new, p0, p1, good_new, mask, old_gray, frame_gray, colors, sparse_y);
        //==================================================================================
        //Optical Flow Evaluation
        Mat next;
        Mat gry = evaluate_dense_opticalflow(next, prvs, frame);
        //Display white ratio in white box on top left corner for optical flow frame
        dPastValue = display_whiteratio_dynamic(gry, frame, dynamic_y, -1);

        imshow("Optical Flow", gry);
        imshow("Original Frame", frame);
        imshow("Foreground Mask", fgMask);
        //imshow("Lucas-Kanade", img_lc);
        // // videoout.write(frame);

        // int keyboard = waitKey(1);
        // if (keyboard == 27)
        //     break;
        prvs = next;
        // Now update the previous frame and previous points
        //old_gray = frame_gray.clone();
        //p0 = good_new;
        //p0.clear();
        //goodFeaturesToTrack(old_gray, p0, 1000, 0.1, 7, Mat(), 7, false, 0.04);
    }
    auto stop = high_resolution_clock::now();
    capture.release();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken by function: "
         << duration.count() / 1000000.0 << " seconds" << endl;
    freopen("../outputs/timetaken.out", "w", stdout);
    cout << duration.count() / 1000000.0 << endl;
    write_out_queue(queue_y);
    write_out_dynamic(dynamic_y);
    destroyAllWindows();
    return 0;
}