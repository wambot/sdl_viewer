#include "ros/ros.h"
#include "std_msgs/ColorRGBA.h"
#include "sensor_msgs/Image.h"
#include "sensor_msgs/CompressedImage.h"
#include "image_transport/image_transport.h"
//#include "rostoy/rostoy.hpp"
#include "SDL/SDL.h"

using namespace std;

SDL_Surface *screen;
int width, height;
std::string encoding;

ros::NodeHandle *node;

void receive_frame(const sensor_msgs::Image::ConstPtr &msg) {
    int x, y;

    //cout << "encoding " << msg->encoding << endl;

    if((int)msg->width != width || (int)msg->height != height || msg->encoding != encoding) {
        width = msg->width;
        height = msg->height;
        encoding = msg->encoding;
        if(screen) {
            SDL_FreeSurface(screen);
            screen = NULL;
        };
        if(encoding == "rgb8" || encoding == "8UC3" || encoding == "bgr8")
            screen = SDL_SetVideoMode(width, height, 24, SDL_HWSURFACE);
        else
            encoding = "invalid";
    };
    //if(encoding == "rgb8") {
    if(encoding == "rgb8" || encoding == "8UC3") {
        for(y = 0; y < screen->h; y++) {
            for(x = 0; x < screen->w; x++) {
                *((uint8_t *)screen->pixels + y * screen->pitch + x * 3 + 0) = msg->data[y * msg->width * 3 + x * 3 + 2];
                *((uint8_t *)screen->pixels + y * screen->pitch + x * 3 + 1) = msg->data[y * msg->width * 3 + x * 3 + 1];
                *((uint8_t *)screen->pixels + y * screen->pitch + x * 3 + 2) = msg->data[y * msg->width * 3 + x * 3 + 0];
            };
        };
    }
    else if(encoding == "bgr8") {
        for(y = 0; y < screen->h; y++) {
            for(x = 0; x < screen->w; x++) {
                *((uint8_t *)screen->pixels + y * screen->pitch + x * 3 + 0) = msg->data[y * msg->width * 3 + x * 3 + 0];
                *((uint8_t *)screen->pixels + y * screen->pitch + x * 3 + 1) = msg->data[y * msg->width * 3 + x * 3 + 1];
                *((uint8_t *)screen->pixels + y * screen->pitch + x * 3 + 2) = msg->data[y * msg->width * 3 + x * 3 + 2];
            };
        };
    };
    /*
    if(encoding == "8UC3") {
        for(y = 0; y < screen->h; y++) {
            for(x = 0; x < screen->w; x++) {
                *((uint8_t *)screen->pixels + y * screen->pitch + x * 3 + 2) = msg->data[y * msg->width * 3 + x * 3 + 2];
                *((uint8_t *)screen->pixels + y * screen->pitch + x * 3 + 1) = msg->data[y * msg->width * 3 + x * 3 + 1];
                *((uint8_t *)screen->pixels + y * screen->pitch + x * 3 + 0) = msg->data[y * msg->width * 3 + x * 3 + 0];
            };
        };
    };
    */
    if(screen)
        SDL_Flip(screen);
};
    
int main(int argc, char **argv) {
    string nodename;
    if(argc > 2)
        ros::init(argc, argv, argv[2], ros::init_options::AnonymousName);
    else
        ros::init(argc, argv, "sdl_viewer", ros::init_options::AnonymousName);

    SDL_Init(SDL_INIT_VIDEO);
    screen = NULL;
    width = -1;
    height = -1;
    encoding = "invalid";

    std::string topic;

    if(argc > 1)
        topic = argv[1];
    else
        topic = "image";
        //topic = "/usb_cam/image_raw";

    node = new ros::NodeHandle();
    nodename = ros::this_node::getName();

    ros::Publisher *pub_color = new ros::Publisher(node->advertise<std_msgs::ColorRGBA>(nodename + "/color_clicked", 1));
    //node->setParam("~image_src/value", topic);
    //node->setParam("~image_src/range", "topic sensor_msgs/Image");
    //node->setParam(nodename + "/image_src/value", topic);
    //node->setParam(nodename + "/image_src/range", "topic sensor_msgs/Image");
    node->setParam(nodename + "/image_src", topic);
    node->setParam(nodename + "/image_src__meta/type", "string");
    node->setParam(nodename + "/image_src__meta/defines", "topic");
    node->setParam(nodename + "/image_src__meta/topic_type", "sensor_msgs/Image");

    //ros::spin();
    image_transport::ImageTransport it(*node);

    std::string old_image_src = "";
    std::string image_src = "";
    ros::Rate r(100);
    image_transport::Subscriber *sub = NULL;
    //ros::Subscriber *sub = NULL;

    for(bool cont = true; cont;) {
        //if(node->getParam(nodename + "/image_src/value", image_src)) {
        if(node->getParam(nodename + "/image_src", image_src)) {
            if(image_src != old_image_src) {
                cout << "image_src = " << image_src << endl;
                if(sub) {
                    delete sub;
                    sub = NULL;
                };
                //sub = new image_transport::Subscriber(node->subscribe(image_src, 1, receive_frame));
                sub = new image_transport::Subscriber(it.subscribe(image_src, 1, receive_frame));
                old_image_src = image_src;
            };
        };
        ros::spinOnce();

        SDL_Event event;
        SDL_PollEvent(&event);
        switch(event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if(screen) {
                uint16_t x, y;
                std_msgs::ColorRGBA out;

                x = event.button.x;
                y = event.button.y;
                out.r = ((uint8_t *)screen->pixels)[y * screen->pitch + x * 3 + 2] / 256.0f;
                out.g = ((uint8_t *)screen->pixels)[y * screen->pitch + x * 3 + 1] / 256.0f;
                out.b = ((uint8_t *)screen->pixels)[y * screen->pitch + x * 3 + 0] / 256.0f;
                out.a = 1.0f;

                cout << "x: " << x << "\ty: " << y << "\tr: " << out.r << "\tg: " << out.g << "\tb: " << out.b << endl;

                pub_color->publish(out);
            };
            break;
        case SDL_QUIT:
            cont = false;
        };

        cont &= ros::ok();
        r.sleep();
    };
    
    node->deleteParam(nodename);
    delete pub_color;

//    subman.manage<sensor_msgs::Image>(sub, "image");

    return 0;
};

