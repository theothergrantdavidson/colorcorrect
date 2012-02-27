#include "cutil.hpp"
#include <vector>
#include <math.h>
#include <cfloat>
#define SATURATION_MAX 255
#define SATURATION_MIN 0

typedef std::vector<double> doubleary;

typedef struct {
  doubleary *rary;
  doubleary *gary;
  doubleary *bary;
  double rsum;
  double gsum;
  double bsum;
} std_t;

typedef struct {
  double **rary;
  double **gary;
  double **bary;
  double rmax;
  double gmax;
  double bmax;
  double rmin;
  double gmin;
  double bmin;
} rscore_t;


double calc_luminance(unsigned char r,unsigned b, unsigned g);
double triangular_function(double luminance);
std_t* calc_standard_deviation(rgbimage_t* img,int subwidth, int subheight, double subblocksize);
double* calc_luminance_weight(rgbimage_t* img,std_t* stdev,int subwidth, int subheight, double subblocksize);
std_t* create_std();
void delete_std(std_t* p);
double** new_2d_mat(rgbimage_t* img);
void delete_2d_mat(double** mat,rgbimage_t* img);
double calc_euclidean(int ax, int ay, int bx, int by);
double calc_inverse_exponential(int ax, int ay, int bx, int by,double alpha);
double calc_manhattan(int ax, int ay, int bx, int by);
double calc_maximum(int ax, int ay, int bx, int by);
double calc_saturation(int diff,double slope);
int get_pixel(unsigned char* ary,rgbimage_t* img, int x, int y);
void set_pixel(unsigned char* ary,rgbimage_t* img, int x, int y,unsigned char c);
rscore_t* create_rscore(rgbimage_t* img);
void delete_rsocre(rscore_t* rs,rgbimage_t* img);

double* calc_sdlwgw(rgbimage_t* img, int subwidth, int subheight){
  double* gains = new double[3]();
  double subblocksize = subwidth*subheight;

  std_t* stdev = calc_standard_deviation(img,subwidth,subheight,subblocksize);
  double* sdlwa = calc_luminance_weight(img, stdev, subwidth, subheight, subblocksize);
  delete_std(stdev);
  double sdlwa_avg = (sdlwa[0]+sdlwa[1]+sdlwa[2])/3.0;
  gains[0] = sdlwa_avg/sdlwa[0];
  gains[1] = sdlwa_avg/sdlwa[1];
  gains[2] = sdlwa_avg/sdlwa[2];
  delete[] sdlwa;
  return gains;
}

double* calc_lwgw(rgbimage_t* img, int subwidth, int subheight){
  double* gains = new double[3]();
  double subblocksize = subwidth*subheight;
  std_t* stdev = create_std();
  for(int i=0;i<img->height;i+=subheight){
    for(int j=0;j<img->width;j+=subwidth){
      stdev->rary->push_back(1.0);
      stdev->gary->push_back(1.0);
      stdev->bary->push_back(1.0);
      stdev->rsum+=1.0;
      stdev->gsum+=1.0;
      stdev->bsum+=1.0;
    }
  }
  double* lwa = calc_luminance_weight(img, stdev, subwidth, subheight, subblocksize);
  delete_std(stdev);
  double lwa_avg = (lwa[0]+lwa[1]+lwa[2])/3.0;
  gains[0] = lwa_avg/lwa[0];
  gains[1] = lwa_avg/lwa[1];
  gains[2] = lwa_avg/lwa[2];
  delete[] lwa;
  return gains;
}

void calc_ace(rgbimage_t* img){

  rscore_t* rs = create_rscore(img);
  //Chromatic/Spatial Adjustment
  for(int i=0;i<img->height;++i){
    for(int j=0;j<img->width;++j){
      int r_pixel = get_pixel(img->r,img,j,i);
      int g_pixel = get_pixel(img->g,img,j,i);
      int b_pixel = get_pixel(img->b,img,j,i);
      double r_rscore_sum = 0.0;
      double g_rscore_sum = 0.0;
      double b_rscore_sum = 0.0;
      //calcurate r score from whole image
      //if you want accelerate the computation, use neighborhood pixels or random sampling insted of whole image.
      for(int k=0;k<img->height;++k){
        for(int l=0;l<img->width;++l){
          if(k==i&&l==j)continue;
          double dist = calc_euclidean(j,i,l,k);
          double r_rscore = calc_saturation(r_pixel - get_pixel(img->r,img,l,k),20.0);
          double g_rscore = calc_saturation(g_pixel - get_pixel(img->g,img,l,k),20.0);
          double b_rscore = calc_saturation(b_pixel - get_pixel(img->b,img,l,k),20.0);
          r_rscore_sum += r_rscore/dist;
          g_rscore_sum += g_rscore/dist;
          b_rscore_sum += b_rscore/dist;
        }
      }
      rs->rary[j][i]=r_rscore_sum;
      rs->gary[j][i]=g_rscore_sum;
      rs->bary[j][i]=b_rscore_sum;
      if(r_rscore_sum > rs->rmax)
        rs->rmax = r_rscore_sum;
      if(g_rscore_sum > rs->gmax)
        rs->gmax = g_rscore_sum;
      if(b_rscore_sum > rs->bmax)
        rs->bmax = b_rscore_sum;
      if(r_rscore_sum < rs->rmin)
        rs->rmin = r_rscore_sum;
      if(g_rscore_sum < rs->gmin)
        rs->gmin = g_rscore_sum;
      if(b_rscore_sum < rs->bmin)
        rs->bmin = b_rscore_sum;
    }
  }

  //Dynamic Tone Reproduction Scaling
  for(int i=0;i<img->height;++i){
    for(int j=0;j<img->width;++j){
      //scaling
    }
  }
  delete_rsocre(rs,img);
}

std_t* calc_standard_deviation(rgbimage_t* img,int subwidth, int subheight, double subblocksize){
  std_t* stdev = create_std();
  //calurete standard deviation for each subblocks
  for(int i=0;i<img->height;i+=subheight){
    for(int j=0;j<img->width;j+=subwidth){
      double avg_r=0;
      double avg_g=0;
      double avg_b=0;
      //iterate pixels in subblock
      for(int k=i;k<subheight;++k){
        for(int l=j;l<subwidth;++l){
          avg_r+=img->r[k*img->width+l];
          avg_g+=img->g[k*img->width+l];
          avg_b+=img->b[k*img->width+l];
        }
      }
      avg_r = avg_r/subblocksize;
      avg_g = avg_g/subblocksize;
      avg_b = avg_b/subblocksize;
      double std_r=0.0;
      double std_g=0.0;
      double std_b=0.0;
      //iterate pixels in subblock
      for(int k=i;k<subheight;++k){
        for(int l=j;l<subwidth;++l){
          std_r+=pow(img->r[k*img->width+l]-avg_r,2.0);
          std_g+=pow(img->g[k*img->width+l]-avg_g,2.0);
          std_b+=pow(img->b[k*img->width+l]-avg_b,2.0);
        }
      }
      std_r = sqrt(std_r/(subblocksize-1.0));
      std_g = sqrt(std_g/(subblocksize-1.0));
      std_b = sqrt(std_b/(subblocksize-1.0));
      stdev->rary->push_back(std_r);
      stdev->gary->push_back(std_g);
      stdev->bary->push_back(std_b);
      stdev->rsum+=std_r;
      stdev->gsum+=std_g;
      stdev->bsum+=std_b;
    }
  }
  return stdev;
}

double* calc_luminance_weight(rgbimage_t* img,std_t* stdev,int subwidth, int subheight, double subblocksize){
  double* sdlwa = new double[3]();
  int bid = 0;
  for(int i=0;i<img->height;i+=subheight){
    for(int j=0;j<img->width;j+=subwidth){
      doubleary wlumi_Ary;
      double wlumi_sum=0.0;
      //iterate pixels in subblock
      for(int k=i;k<subheight;++k){
        for(int l=j;l<subwidth;++l){
          unsigned char r=img->r[k*img->width+l];
          unsigned char g=img->g[k*img->width+l];
          unsigned char b=img->b[k*img->width+l];
          double luminance = calc_luminance(r,g,b);
          double w_luminance = triangular_function(luminance);
          wlumi_sum+=w_luminance;
          wlumi_Ary.push_back(w_luminance);
        }
      }
      int subpid=0;
      double lumi_r = 0;
      double lumi_g = 0;
      double lumi_b = 0;
      //iterate pixels in subblock
      for(int k=i;k<subheight;++k){
        for(int l=j;l<subwidth;++l){
          lumi_r += (wlumi_Ary[subpid]/wlumi_sum)*img->r[k*img->width+l];
          lumi_g += (wlumi_Ary[subpid]/wlumi_sum)*img->g[k*img->width+l];
          lumi_b += (wlumi_Ary[subpid]/wlumi_sum)*img->b[k*img->width+l];
          ++subpid;//increment pixel id
        }
      }
      sdlwa[0] += (stdev->rary->at(bid)/stdev->rsum)*lumi_r;
      sdlwa[1] += (stdev->gary->at(bid)/stdev->gsum)*lumi_g;
      sdlwa[2] += (stdev->bary->at(bid)/stdev->bsum)*lumi_b;
      ++bid;//increment block id
    }
  }
  return sdlwa;
}

double calc_luminance(unsigned char r,unsigned b, unsigned g){
  return (0.298912*r+0.586611*g+0.114478*b);
}

double triangular_function(double luminance){
  if(luminance<160){
    return luminance*(1.0/160.0);
  }else{
    return 1.0-(luminance-160)*(1.0/160.0);
  }
}

std_t* create_std(){
  std_t* stdev = new std_t;
  stdev->rary = new doubleary;
  stdev->gary = new doubleary;
  stdev->bary = new doubleary;
  stdev->rsum = 0.0;
  stdev->gsum = 0.0;
  stdev->bsum = 0.0;
  return stdev;
}

void delete_std(std_t* p){
  delete p->rary;
  delete p->gary;
  delete p->bary;
  delete p;
}

rscore_t* create_rscore(rgbimage_t* img){
  rscore_t* rs = new rscore_t;
  rs->rary = new_2d_mat(img);
  rs->gary = new_2d_mat(img);
  rs->bary = new_2d_mat(img);
  rs->rmax = DBL_MIN;
  rs->gmax = DBL_MIN;
  rs->bmax = DBL_MIN;
  rs->rmin = DBL_MAX;
  rs->gmin = DBL_MAX;
  rs->bmin = DBL_MAX;
  return rs;
}

void delete_rsocre(rscore_t* rs,rgbimage_t* img){
  delete_2d_mat(rs->rary,img);
  delete_2d_mat(rs->gary,img);
  delete_2d_mat(rs->bary,img);
  delete rs;
}

void delete_doubleptr(double* p){
  delete[] p;
}

double** new_2d_mat(rgbimage_t* img){
  double** mat = new double*[img->width];
  for(int i=0; i<img->width; ++i){
    mat[i] = new double[img->height]();
  }
  return mat;
}

void delete_2d_mat(double** mat,rgbimage_t* img){
  for(int i = 0;i< img->width;++i){
    delete[] mat[i];
  }
  delete[] mat;
}

double calc_euclidean(int ax, int ay, int bx, int by){
  return sqrt(pow(ax-bx,2.0)+pow(ay-by,2.0));
}

double calc_inverse_exponential(int ax, int ay, int bx, int by,double alpha){
  return 1.0/exp(calc_euclidean(ax,ay,bx,by)*-alpha);
}


double calc_manhattan(int ax, int ay, int bx, int by){
  return (double)(fabs(ax-bx)+fabs(ay-by));
}

double calc_maximum(int ax, int ay, int bx, int by){
  return ((fabs(ax-bx)>fabs(ay-by))? fabs(ax-bx):fabs(ay-by));
}

double calc_saturation(int diff,double slope){
  double ret = diff*slope;
  if(ret>SATURATION_MAX){
    return SATURATION_MAX;
  }else if(ret < SATURATION_MIN){
    return  SATURATION_MIN;
  }else{
    return ret;
  }
}

int get_pixel(unsigned char* ary,rgbimage_t* img, int x, int y){
  return (int)ary[y*img->width+x];
}

void set_pixel(unsigned char* ary,rgbimage_t* img, int x, int y,unsigned char c){
  ary[y*img->width+x]=c;
}
