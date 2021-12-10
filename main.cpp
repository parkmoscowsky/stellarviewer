#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <cmath>
#include <windows.h>
#include <fstream>
#include <set>
#include <algorithm>

#define PI 3.14159265358979323846

struct Dot {
    int x, y, sc;
    char shape;

    Dot(int x, int y, double m, int sc, double lower_magnitude): x(x), y(y), sc(sc) {
        if(m > lower_magnitude-1) shape='.';
        else if(m > lower_magnitude-2) shape='*';
        else if(m > lower_magnitude-3) shape='#';
        else shape='@';
    }
};

struct Vector3D {
    double x, y, z;

    Vector3D(): Vector3D(0, 0, 0) {}

    Vector3D(double x, double y, double z): x(x), y(y), z(z) {}

    Vector3D(Vector3D& v): x(v.x), y(v.y), z(v.z) {}

    Vector3D operator+=(Vector3D& v) {
        this->x += v.x;
        this->y += v.y;
        this->z += v.z;
        return *this;
    }

    double operator*(Vector3D& v) {
        return this->x*v.x + this->y*v.y + this->z*v.z;
    }

    Vector3D operator*=(double f) {
        this->x *= f;
        this->y *= f;
        this->z *= f;
        return *this;
    }
};

class Sky {
public:
    double ra, d, angle;
    std::string file_name;

    Sky(std::string file_name): ra(0), d(0), angle(1), file_name(file_name) {}

    void catch_command(bool&);
};

bool DotVectorComparator(Dot a, Dot b) {
    int a_shape, b_shape;
    if(a.shape=='.') a_shape=3; else if(a.shape=='*') a_shape=2; else if(a.shape=='#') a_shape=1; else a_shape=0;
    if(b.shape=='.') b_shape=3; else if(b.shape=='*') b_shape=2; else if(b.shape=='#') b_shape=1; else b_shape=0;

    return a.y < b.y || (a.y==b.y && a.x < b.x) || (a.y==b.y && a.x==b.x && a_shape < b_shape);
}

struct DotSetComparator {
    bool operator()(const Dot& a, const Dot& b) const {
        return a.y < b.y || (a.y==b.y && a.x < b.x);
    }
};

class Projection {
private:
    std::set<Dot,DotSetComparator> stars;
public:
    Projection(Sky& sky) {
        std::vector<Dot> dots;
        double lower_magnitude = 6.87 - 3.0 * sky.angle;
        double ra, d, m, cosg, sing, tanr2, nstsk, npn;
        char sc;
        int sc_, x, y;
        bool correct;

        std::string s;
        std::ifstream data(sky.file_name);

        if(data.is_open()) {
            while(getline(data, s)) {
                    correct = true;
                try {
                m = std::stod(s.substr(102,5));
                } catch(...) {
                    correct = false;
                }
                if(correct&&m<=lower_magnitude) {
                    sc = s[129];

                    if(sc=='M') sc_=4;
                    else if(sc=='K') sc_=12;
                    else if(sc=='G') sc_=6;
                    else if(sc=='F') sc_=14;
                    else if(sc=='A') sc_=15;
                    else if(sc=='B') sc_=3;
                    else if(sc=='O') sc_=9;
                    else correct = false;

                    if(correct) {
                    ra = std::stod(s.substr(75,2))*15+std::stod(s.substr(77,2))*0.25+std::stod(s.substr(79,4))/240;
                    d = std::stod(s.substr(83,3))+std::stod(s.substr(86,2))/60+std::stod(s.substr(88,2))/3600;
                    ra *= PI/180;
                    d *= PI/180;

                    Vector3D nst(cos(d)*cos(ra), cos(d)*sin(ra), sin(d));
                    Vector3D nsk(cos(sky.d)*cos(sky.ra), cos(sky.d)*sin(sky.ra), sin(sky.d));
                    Vector3D dth(-sin(sky.d)*cos(sky.ra), -sin(sky.d)*sin(sky.ra), cos(sky.d));
                    Vector3D dph(-sin(sky.ra), cos(sky.ra), 0);

                    nstsk = nst*nsk;
                    Vector3D nnsk(nsk);
                    nnsk *= -nstsk;
                    Vector3D np(nst);
                    np += nnsk;
                    npn = sqrt(np*np);
                    np *= 1/npn;

                    cosg = dth*np;
                    sing = dph*np;
                    tanr2 = sqrt((1-nstsk)/(1+nstsk));
                    tanr2 *= 25/sky.angle;

                    x = 50 - 2*tanr2*sing;
                    y = 25 - tanr2*cosg;
                    if(tanr2<=25) dots.push_back(Dot(x, y, m, sc_, lower_magnitude));
                }
            }
        }
        data.close();

        std::sort(dots.begin(),dots.end(),DotVectorComparator);
        for(auto pos = dots.begin();pos!=dots.end();++pos)
            stars.insert(*pos);
        }
    }

    friend std::ostream& operator<<(std::ostream &out, const Projection& proj) {
        HANDLE hConsole;
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        auto iter = proj.stars.begin();
        SetConsoleTextAttribute(hConsole, 15);

        for(int y_pos=0;y_pos!=50;++y_pos) {
            for(int x_pos=0;x_pos!=100;++x_pos) {
                if(x_pos==(*iter).x&&y_pos==(*iter).y) {
                    SetConsoleTextAttribute(hConsole, (*iter).sc);
                    std::cout<<(*iter).shape;
                    ++iter;
                }
                else std::cout<<" ";
            }
            std::cout<<"\n";
        }
        SetConsoleTextAttribute(hConsole, 15);
        return out;
    }
};

void Sky::catch_command(bool& active) {
        std::string command;
        std::getline(std::cin, command);

        if(command=="help") {
            std::cout<<"Welcome to StellarViewer Help Manager!\nHere you can find some information about the program itself and how to use it\nabout/commands: ";
            std::string answer;
            std::getline(std::cin,answer);

            if(answer=="about") {
                std::cout<<"StellarViewer is my 3rd term mini-project for IT classes. It is a program that can show you part of the sky ";
                std::cout<<"centered on a particular point on the stellar sphere with given viewing angle. Program works with database ";
                std::cout<<"consisting of more than 9000 brightest stars, and the narrower your viewing angle is, the more stars you will be ";
                std::cout<<"able to see. Stars are colored according to their spectral classes, and their shape at the output depends on their ";
                std::cout<<"visible magnitude. Program operates fully in console. You can use console commands to turn your viewing point on ";
                std::cout<<"the night sky in particular direction, zoom in and zoom out, and also manually set corresponding parameters. If you ";
                std::cout<<"want to know more about console commands, enter help->commands.\n\n";
            }
            else if(answer=="commands") {
                std::cout<<"StellarViewer console commands:\n\n\nhelp\nGuides you with all the information about StellarViewer.\n\nconfig\n\nShows you ";
                std::cout<<"current configuration of your viewing point on the sky (i.e. its right ascension, declination and viewing angle)\n\nzoom in\n\n";
                std::cout<<"Makes your viewing angle smaller, but no less than 11 degrees.\n\nzoom out\n\nMakes your viewing angle bigger";
                std::cout<<", but no more than 90 deg.\n\nleft\n\nTurns your viewing point on the stellar sphere left.\n\nright\n\nTurns your ";
                std::cout<<"viewing point on the stellar sphere right.\n\nup\n\nTurns your viewing point on the stellar sphere up, but no more than ";
                std::cout<<"90 degrees.\n\ndown\n\nTurns your viewing point on the stellar sphere down, but no more than -90 degrees.\n\nset right ";
                std::cout<<"ascension\n\nSets right ascension of your viewing point in hours and minutes.\n\nset declination\n\nSets declination of ";
                std::cout<<"your viewing point in degrees.\n\nset viewing angle\n\nSets your viewing angle in degrees.\n\nexit\n\nThe ";
                std::cout<<"command to exit StellarViewer.\n\n";
            }
            else std::cout<<"Unknown command! Please try again.\n\n";
        }
        else if(command=="exit") {
            std::cout<<"\nAre you sure you want to exit StellarViewer?\ny/n: ";
            std::string answer;
            std::getline(std::cin,answer);

            if(answer=="y") {
                std::cout<<"\nExiting StellarViewer...\n\n";
                active = false;
            }
            else if(answer=="n") std::cout<<"\nGlad to know you want to continue!\n\n";
            else std::cout<<"\nUnknown command! Please try again.\n\n";
        }
        else if(command=="zoom in") {
            if(angle>=0.125&&angle<=1) angle *= 0.8;
            else angle = 0.1;
            std::cout<<"\nYour viewing angle was changed to "<<(int)(360*atan(angle)/PI)<<" degrees.\n\n";
        }
        else if(command=="zoom out") {
            if(angle>=0.1&&angle<=0.8) angle *= 1.25;
            else angle = 1.0;
            std::cout<<"\nYour viewing angle was changed to "<<(int)(360*atan(angle)/PI)<<" degrees.\n\n";
        }
        else if(command=="show") {
            std::cout<<"\nPreparing to show you sky...\n\n"<<Projection(*this)<<"\n";
        }
        else if(command=="up") {
            d += 0.5*angle;
            if(d>PI/2) d = PI/2;
            std::cout<<"\nPoint of view turned up to new declination of "<<(int)(d*180/PI)<<" degrees.\n\n";
        }
        else if(command=="down") {
            d -= 0.5*angle;
            if(d<-PI/2) d = -PI/2;
            std::cout<<"\nPoint of view turned down to new declination of "<<(int)(d*180/PI)<<" degrees.\n\n";
        }
        else if(command=="left") {
            ra += 0.5*angle;
            int ra_h = ra*12/PI;
            int ra_m = ra*720/PI-60*ra_h;
            std::cout<<"\nPoint of view turned left to new right ascension of "<<ra_h<<" hours "<<ra_m<<" minutes.\n\n";
        }
        else if(command=="right") {
            ra -= 0.5*angle;
            int ra_h = ra*12/PI;
            int ra_m = ra*720/PI-60*ra_h;
            std::cout<<"\nPoint of view turned right to new right ascension of "<<ra_h<<" hours "<<ra_m<<" minutes.\n\n";
        }
        else if(command=="set right ascension") {
            std::cout<<"\nEnter right ascension in hours and minutes.\n\nhours: ";
            try {
                std::string ra_h;
                std::string ra_m;
                std::getline(std::cin, ra_h);
                std::cout<<"\nminutes: ";
                std::getline(std::cin,ra_m);
                double new_ra_h = std::stoi(ra_h), new_ra_m = std::stoi(ra_m);
                if(new_ra_h>=0&&new_ra_h<24&&new_ra_m>=0&&new_ra_m<60) {
                    ra = (new_ra_h/12+new_ra_m/720)*PI;
                    std::cout<<"\nNew right ascension is set.\n\n";
                }
                else std::cout<<"\nYou have entered data in inappropriate format! Please try again.\n\n";
            } catch(...) {
                std::cout<<"\nYou have entered data in inappropriate format! Please try again.\n\n";
            }
        }
        else if(command=="set declination") {
            std::cout<<"\nEnter declination in degrees.\n\n";
            try {
                std::string decl;
                std::getline(std::cin, decl);
                double new_d = std::stoi(decl);
                if(new_d>=-90&&new_d<=90) {
                    d = new_d*PI/180;
                    std::cout<<"\nNew declination is set.\n\n";
                }
                else std::cout<<"\nYou have entered data in inappropriate format! Please try again.\n\n";
            } catch(...) {
                std::cout<<"\nYou have entered data in inappropriate format! Please try again.\n\n";
            }
        }
        else if(command=="set viewing angle") {
            std::cout<<"\nEnter viewing angle in degrees.\n\n";
            try {
                std::string a;
                std::getline(std::cin, a);
                double new_a = std::stoi(a);
                if(new_a>0&&new_a<12) {
                    angle = 0.1;
                    std::cout<<"\nNew declination is set.\n\n";
                }
                else if(new_a>=12&&new_a<=90) {
                    angle = new_a*PI/180;
                    std::cout<<"\nNew declination is set.\n\n";
                }
                else std::cout<<"\nYou have entered data in inappropriate format! Please try again.\n\n";
            } catch(...) {
                std::cout<<"\nYou have entered data in inappropriate format! Please try again.\n\n";
            }
        }
        else if(command=="config") {
            std::cout<<"\nHere is current state of the program:\n\n";
            int ra_h = ra*12/PI;
            int ra_m = ra*720/PI-60*ra_h;
            std::cout<<"Right ascension is "<<ra_h<<" hours "<<ra_m<<" minutes.\n";
            std::cout<<"Declination is "<<(int)(d*180/PI)<<" degrees.\n";
            std::cout<<"Viewing angle is "<<(int)(2*atan(angle)*180/PI)<<" degrees.\n\n";
        }
        else std::cout<<"\nUnknown command! Please try again.\n\n";
}

int main() {
    std::cout<<"Welcome to StellarViewer!\nInitializing databases...\n";
    bool is_running = true;
    std::string file = "data.DAT";
    Sky main_sky(file);
    std::cout<<"Databases are initialized.\nEnjoy sky watching! Enter help if you need a guide to StellarViewer implementation.\n\n";

    while(is_running) {
        main_sky.catch_command(is_running);
    }

    return 0;
}
