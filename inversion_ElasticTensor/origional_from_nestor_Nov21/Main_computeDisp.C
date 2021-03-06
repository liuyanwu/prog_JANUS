// this is for computing dispersion curve based on an imput model
// pay attention to the included CALMineos, the "computeDisp4IsoVs" version is used to compute disp correlated with Isotropic Vs, not anisotropic model in which Vsv&Vsh are different
//

#include<iostream>
#include<algorithm>
#include<vector>
#include<cmath>
#include<fstream>
#include <Eigen/Core>
#include<string>

using namespace std;
using namespace Eigen;

#include"./string_split.C"
#include"./generate_Bs.C"
#include"./gen_random.C"
#include"./INITstructure.h"
#include"./CALpara_isolay.C"
#include"./CALgroup_smooth.C"
#include"CALmodel_LVZ_ET.C"
//#include"CALforward_Mineos_readK.C_changeING"
#include"CALforward_Mineos_readK.C_bak"
#include "./ASC_rw.C"
//#include "./ASC_rw.C_v2" // amp is in % rather than absolute value
#include "./BIN_rw.C"

#define _USE_MATH_DEFINES

/*
#include "./BIN_rw.C"
#include "./CALinv_isolay.C"
#define PI 3.14159265
*/



int main(int argc, char *argv[])
{
int npoint,Rsurflag,Lsurflag,Rmonoc,Lmonoc,PosAnic,iitercri1,iitercri2,ijumpcri1,ijumpcri2;
int i,j,k,isoflag,Nprem,k1,k2;
int AziampRsurflag,AziphiRsurflag,AziampLsurflag,AziphiLsurflag;
double bestmisfit, misfit, L,misfitcri;
float depcri1,depcri2,qpcri,qscri,lon,lat;
char inponm[100],Rgpindir[100],Rphindir[100],Lphindir[100],Lgpindir[100],kernelnmR[100],kernelnmL[100];
vector<string> AziampRdispnm,AziphiRdispnm,AziampLdispnm,AziphiLdispnm;
vector<string> Rdispnm,Ldispnm;
char nodeid[5],str[150],modnm[100],Lparanm[100],Rparanm[100],PREMnm[100],fparanm[100];
time_t start;
modeldef model0,model1,ttmodel,modelref,modelnew;
modeldef modeltemp;
paradef para0,para1,para2,pararef,paranew;
FILE *inpo,*fkernel,*fmisfit;
vector<vector<double> >  PREM;
vector<vector<vector<double> > > Vkernel,Lkernel;
vector<int> Lvmono,Lvgrad,Rvmono,Rvgrad,Vposani,idlst;
vector<paradef> paralst;
vector<double> Rparastd,Lparastd;
char modnm1[100],fRdispnm[100],fLdispnm[100],modnm2[100],fAZRdispnm[100],fAZLdispnm[100];
char outinitnm[100],lay[20],dirlay[50],tmpstr[500],fbinnm1[200],fbinnm2[200];

if(argc!=9){
printf("Usage: xx 1]input_point_file 2]output_dir_name 3]input_vsv_dir_name 4]Rphindir 5]Rgpindir 6]Lphindir 7]Lgindir\n");
printf("1] node_id node_lon node_lat\n3]dir/initmod/vsv_node_lon_lat.mod\n");
exit(0);
}
sprintf(dirlay,argv[2]);

  //----------------PARAMETERS-----------------------------------------
  isoflag=0; //isoflag==1: Vsv=Vsh, isoflag==0: Vsv!=Vsh
  Rsurflag=1; //surflag==1: open phase only. surfalg ==3 open phase and group, surflag==2: open group only
  Lsurflag=1;
  AziampRsurflag=1;
  AziphiRsurflag=1;
  AziampLsurflag=1;
  AziphiLsurflag=1;
  iitercri1=10000;
  iitercri2=15000;
  ijumpcri1=9;
  ijumpcri2=4;
  depcri1=20.0;
  depcri2=80.0;
  qpcri=900.;//900.;
  qscri=250.;
  Rmonoc=1;
  Lmonoc=1;
  PosAnic=0;
  //Rvmono.push_back(0);
 // Rvmono.push_back(1);
  Lvmono.push_back(1);
  //Rvgrad.push_back(0);
  Rvgrad.push_back(1);//Rvgrad.push_back(2);
  //Lvgrad.push_back(0);
  Lvgrad.push_back(1);
  //Vposani.push_back(1);
  //Vposani.push_back(1);Vposani.push_back(2);
  k1=0;k2=1;
  //----------------------------------------------------------------------

  //sprintf(PREMnm,"/home/jiayi/progs/jy/Mineos/Mineos-Linux64-1_0_2/DEMO/models/prem_noocean.txt");
  sprintf(PREMnm,"/home/jiayi/progs/jy/Mineos/Mineos-Linux64-1_0_2/DEMO/models/ak135_iso_nowater.txt");
  sprintf(inponm,argv[1]);
  sprintf(Rphindir,argv[4]);
  sprintf(Rgpindir,argv[5]);
  sprintf(Lphindir,argv[6]);
  sprintf(Lgpindir,argv[7]);
  sprintf(fparanm,argv[8]);

  readPREM(PREMnm,PREM,Nprem);

  sprintf(tmpstr,"if [ ! -d %s ]; then mkdir %s; fi",dirlay,dirlay);
  system(tmpstr);
  sprintf(tmpstr,"if [ ! -d %s/initmod ]; then mkdir %s/initmod; fi",dirlay,dirlay);
  system(tmpstr);
sprintf(tmpstr,"if [ ! -d %s/binmod ]; then mkdir %s/binmod; fi",dirlay,dirlay);
  system(tmpstr);

  //---------------------------------------------------------
  if((inpo=fopen(inponm,"r"))==NULL){
	printf("Cannot open points file %s!\n",inponm);exit(0);
  }
  npoint=0;

  while(1){
    if(fscanf(inpo,"%s %f %f",&nodeid[0],&lon,&lat)==EOF)
	break;
    npoint++;
    printf("Begin to work on point %d: id=%s lon=%f lat=%f\n",npoint,nodeid,lon,lat);

    start=time(0);
    bestmisfit=1e10;

    Rdispnm.clear();
    sprintf(str,"%s/disp_%.1f_%.1f.txt",Rphindir,lon,lat);
    Rdispnm.push_back(str);

    Ldispnm.clear();
    sprintf(str,"%s/disp_%.1f_%.1f.txt",Lphindir,lon,lat);
    Ldispnm.push_back(str);

    AziampRdispnm.clear();
    sprintf(str,"%s/aziamp_%.1f_%.1f.txt",Rphindir,lon,lat);
    AziampRdispnm.push_back(str);

    AziphiRdispnm.clear();
    sprintf(str,"%s/aziphi_%.1f_%.1f.txt",Rphindir,lon,lat);
    AziphiRdispnm.push_back(str);

    AziampLdispnm.clear();
    sprintf(str,"%s/aziamp_%.1f_%.1f.txt",Lphindir,lon,lat);
    AziampLdispnm.push_back(str);

    AziphiLdispnm.clear();
    sprintf(str,"%s/aziphi_%.1f_%.1f.txt",Lphindir,lon,lat);
    AziphiLdispnm.push_back(str);



    //-----the final output model, that will serve as input model for the next step 
    sprintf(outinitnm,"%s/initmod/ani_%s_%.1f_%.1f.mod",dirlay,nodeid,lon,lat);
    //-----the outpur binary files, all the accepted models during inversion
    sprintf(fbinnm1,"%s/binmod/ani_%s_%.1f_%.1f.bin",dirlay,nodeid,lon,lat);
    sprintf(fbinnm2,fbinnm1);//overwrite the first binary output


    //-----starting model-----
    sprintf(modnm,argv[3]);
    //---------------------------------------------------------
    initmodel(model0);
    initmodel(modelref);

    initpara(para0);
    initpara(para1);
    initpara(pararef);
  
    readdisp(model0,Rdispnm,Ldispnm,AziampRdispnm,AziphiRdispnm,AziampLdispnm,AziphiLdispnm,Rsurflag,Lsurflag,AziampRsurflag,AziphiRsurflag,AziampLsurflag,AziphiLsurflag); 
    //==check===
    printf("the Rayleigh wave data:\n");
    for(i=0;i<model0.data.Rdisp.npper;i++){
    	printf("T=%g vel=%g unc=%g\n",model0.data.Rdisp.pper[i],model0.data.Rdisp.pvelo[i],model0.data.Rdisp.unpvelo[i]);
    }
    printf("the Rayleigh Azi wave data:\n");
    for(i=0;i<model0.data.AziampRdisp.npper;i++){
    	printf("T=%g vel=%g unc=%g\n",model0.data.AziampRdisp.pper[i],model0.data.AziampRdisp.pvelo[i],model0.data.AziampRdisp.unpvelo[i]);
    }
    //
    
    readmodAniso(model0,modnm);// both m.g.LV/Rv are filled regardless of flags. (readin iso model)
    printf("#########group[0].vsvvalue[0]=%g vshvalue[0]=%g\n",model0.groups[0].vsvvalue[0],model0.groups[0].vshvalue[0]);
    //===check===
    for(i=0;i<model0.ngroup;i++){
    	printf("group %d\n",i);
	for(j=0;j<model0.groups[i].np;j++){
		printf("  np %d\n vsv=%g vsh=%g vpv=%g vph=%g eta=%g theta=%g phi=%g vpvs=%g\n",j,model0.groups[i].vsvvalue[j],model0.groups[i].vshvalue[j],model0.groups[i].vpvvalue[j],model0.groups[i].vphvalue[j],model0.groups[i].etavalue[j],model0.groups[i].thetavalue[j],model0.groups[i].phivalue[j],model0.groups[i].vpvs);
	}
    
    }
    //
    readpara(para0,fparanm);


    mod2para(model0,para0,para1);////fill both para.R/Lpara0 (they could be inequal if Rf*Lf>0, they are equal if Rf*Lf=0)

    checkParaModel(para1,model0);
    //===check===
    printf("check readpara & mod2para----\n");
    printf("inpara.flag=%d\n",para0.flag);
    for(i=0;i<para1.npara;i++){
	printf("parameter %d\n",i);
	printf(" value=%g  dv=%g ng=%g nv=%g pflag=%g LVflag=%g RWflag=%g LWflag=%g AZflag=%g\n",para1.parameter[i],para1.para0[i][2],para1.para0[i][4],para1.para0[i][5],para1.para0[i][6],para1.para0[i][7],para1.para0[i][8],para1.para0[i][9],para1.para0[i][10]);
    }
    //
    printf("###@@@ check Eta: p.eta=%g m.g.eta=%g\n",para1.parameter[5],model0.groups[1].etavalue[0]);
    int flagupdaterho=0;
    Vpara2Lovepara(para1,model0,flagupdaterho);//there is para2mod inside the code

    /*--check---
    printf("F=%g = (%g)*(%g-2*%g)=%g\n",para1.LoveRAparameter[5],para1.parameter[5],para1.LoveRAparameter[4],para1.LoveRAparameter[1],para1.parameter[5]*(para1.LoveRAparameter[4]-2*para1.LoveRAparameter[1]));
    printf("###@@@ check Eta: p.eta=%g m.g.eta=%g\n",para1.parameter[5],model0.groups[1].etavalue[0]);
    exit(0);
    */

    int flagreadVkernel=0,flagreadLkernel=0;

    printf("---test compute_dispM\n");
    if(model0.flag==0){updatemodel(model0,flagupdaterho);}
    compute_dispMineos(model0,PREM,Nprem,Rsurflag,Lsurflag);
    printf("---test finish compute_dispM\n");

    //exit(0);
    printf("@@@ check, main, from initial model\n");
    for(j=0;j<model0.data.Rdisp.npper;j++)printf("  @@@ check, T=%g,vin=%g vold=%g\n",model0.data.Rdisp.pper[j],model0.data.Rdisp.pvelo[j],model0.data.Rdisp.pvel[j]);

    printf("@@@ check, do V kernel ====\n");
    //---obtain V kernel ---
    sprintf(kernelnmR,"%s/VkernelRp1ani_%s_%.1f_%.1f.txt",dirlay,nodeid,lon,lat);
    sprintf(kernelnmL,"%s/VkernelLp1ani_%s_%.1f_%.1f.txt",dirlay,nodeid,lon,lat);

    if(flagreadVkernel==1){
    	if((read_kernel(para1,model0,Vkernel,kernelnmR,kernelnmL,Rsurflag,Lsurflag,PREM,Nprem))==0){
		 printf ("#####!! read_kernel failed\n");
    	 	 sprintf(str,"echo point %d: id=%s lon=%f lat=%f >> point_rdKernel_failed_Ani.txt",npoint,nodeid,lon,lat);
         	 system(str);
		 continue;
     	} // if readkernel()==0    
    }//if flagreadkernel==1
    else{
    	compute_Vkernel(para1,model0,Vkernel,PREM,Nprem,Rsurflag,Lsurflag,flagupdaterho);
	cout<<"check finish compute_Vkernel\n";
	write_kernel(Vkernel,model0,para1,kernelnmR,kernelnmL,Rsurflag,Lsurflag);
	cout<<"check finish write_Vkernel\n";
    }//else flagreadkernel==1
    //====


    printf("@@@ check, do L kernel ====\n");
    //---obtain Love kernel ---
    sprintf(kernelnmR,"%s/LkernelRp1ani_%s_%.1f_%.1f.txt",dirlay,nodeid,lon,lat);
    sprintf(kernelnmL,"%s/LkernelLp1ani_%s_%.1f_%.1f.txt",dirlay,nodeid,lon,lat);
    if(flagreadLkernel==1){
    	if((read_kernel(para1,model0,Lkernel,kernelnmR,kernelnmL,Rsurflag,Lsurflag,PREM,Nprem))==0){
		 printf ("#####!! read_kernel failed\n");
    	 	 sprintf(str,"echo point %d: id=%s lon=%f lat=%f >> point_rdKernel_failed_Ani.txt",npoint,nodeid,lon,lat);
         	 system(str);
		 continue;
     	} // if readkernel()==0    
    	
    }
    else{
    	Vkernel2Lkernel(para1,model0,Vkernel,Lkernel,flagupdaterho);
    	write_kernel(Lkernel,model0,para1,kernelnmR,kernelnmL,Rsurflag,Lsurflag);
    }
	
    modelref=model0;
    pararef=para1;

    float theta;
    for(theta=90.;theta<100;theta=theta+30.){

    printf("============== working on theta=%.0f =======\n",theta);	    

    modelnew=model0;
    paranew=para1;
    //paranew.parameter[6]=90.;
    //modelnew.groups[1].vshvalue[1]=modelnew.groups[1].vshvalue[1]*1.01;
    modelnew.groups[1].thetavalue[0]=theta;
//76.2203 81.6582
    //modelnew.groups[1].thetavalue[0]=90.;
    modelnew.groups[1].phivalue[0]=-45.;
    //modelnew.groups[1].thetavalue[2]=theta;
    //--- if group.flagcpttype==3, then theta--> Acos, phi--> Asin of the dVsv (sqrt(Acos^2+Asin^2)=amplitude of azimuthal anisotropy);

    //modelnew.groups[2].thetavalue[0]=0.01*modelnew.groups[2].vsvvalue[0];
    //modelnew.groups[2].phivalue[0]=0.005*modelnew.groups[2].vsvvalue[0];

    mod2para(modelnew,para1,paranew);
    //para2mod(paranew,model0,modelnew);
    cout<<"hey"<<modelnew.groups[1].thetavalue[0]<<"  "<<paranew.parameter[6]<<endl;
    Vpara2Lovepara(paranew,modelnew,flagupdaterho); // there is para2mod inside the code
    cout<<"hey"<<modelnew.groups[1].thetavalue[0]<<"  "<<paranew.parameter[6]<<endl;

    for(int n =0;n<modelnew.ngroup;n++){
	for(int m=0;m<modelnew.groups[n].np;m++)
	    printf("theta in gp%d, nv%d=%g phi=%g\n",n,m,modelnew.groups[n].thetavalue[m],modelnew.groups[n].phivalue[m]);
    }

    //---test----
    printf("vsv=%7f vsh=%7f vpv=%7f vph=%7f eta=%7f\n",modelnew.groups[1].vsvvalue[1],modelnew.groups[1].vshvalue[1],modelnew.groups[1].vpvvalue[1],modelnew.groups[1].vphvalue[1],modelnew.groups[1].etavalue[1]);
    model0=modelnew;
    Lovepara2Vpara(paranew,model0);
    para0=paranew;
    para2mod_static(paranew,model0,modelnew);	    
    //para2mod(para0,model0,modelnew);
    printf("vsv=%7f vsh=%7f vpv=%7f vph=%7f eta=%7f\n",modelnew.groups[1].vsvvalue[1],modelnew.groups[1].vshvalue[1],modelnew.groups[1].vpvvalue[1],modelnew.groups[1].vphvalue[1],modelnew.groups[1].etavalue[1]);
    //-----------
    compute_dispKernel(modelnew,paranew,modelref,pararef,Vkernel,Lkernel,Rsurflag,Lsurflag,max(AziampRsurflag,AziphiRsurflag),max(AziampLsurflag,AziphiLsurflag));
 
    //===check==
    printf("@@@ check, main, compute_dispKernel\n");
    /*
    for(i=0;i<modelnew.data.Rdisp.npper;i++){
    	printf("  the %dth phvel Rayleigh:RA vref=%5g vnew=%5g   AZ ampin=%5g ampnew=%5g; phiin=%5g phinew=%5f\n",i,modelref.data.Rdisp.pvel[i],modelnew.data.Rdisp.pvel[i],modelref.data.AziampRdisp.pvelo[i],modelnew.data.AziampRdisp.pvel[i],modelref.data.AziphiRdisp.pvelo[i],modelnew.data.AziphiRdisp.pvel[i]);
    }

    for(i=0;i<modelnew.data.Ldisp.npper;i++){
    	printf("  the %dth phvel Love:RA vref=%5g vnew=%5g   AZ ampin=%5g ampnew=%5g; phiin=%5g phinew=%5f\n",i,modelref.data.Ldisp.pvel[i],modelnew.data.Ldisp.pvel[i],modelref.data.AziampLdisp.pvelo[i],modelnew.data.AziampLdisp.pvel[i],modelref.data.AziphiLdisp.pvelo[i],modelnew.data.AziphiLdisp.pvel[i]);
    }
    */
    //--write out disp ----------
    sprintf(fRdispnm,"%s/Rdisp_%.0f_%s_%.1f_%.1f.txt",theta,dirlay,nodeid,lon,lat);
    sprintf(fLdispnm,"%s/Ldisp_%.0f_%s_%.1f_%.1f.txt",theta,dirlay,nodeid,lon,lat);
    sprintf(fAZRdispnm,"%s/AZRdisp_%.0f_%s_%.1f_%.1f.txt",theta,dirlay,nodeid,lon,lat);
    sprintf(fAZLdispnm,"%s/AZLdisp_%.0f_%s_%.1f_%.1f.txt",theta,dirlay,nodeid,lon,lat);
    write_ASC(modelnew,paranew,fRdispnm,fLdispnm,fAZRdispnm,fAZLdispnm,Rsurflag,Lsurflag,AziampRsurflag,AziampLsurflag,AziphiRsurflag,AziphiLsurflag);

    ///*
    //--compute the TI part with Mineos----
    //--get the effective TI vsv~eta for the rotated medium---
    Lovepara2Vpara(paranew,modelnew);
    modeltemp=modelnew;
    para2mod(paranew,modeltemp,modelnew);
    updatemodel(modelnew,flagupdaterho);
    compute_dispMineos(modelnew,PREM,Nprem,Rsurflag,Lsurflag);
    sprintf(fRdispnm,"%s/RdispM_%.0f_%s_%.1f_%.1f.txt",theta,dirlay,nodeid,lon,lat);
    sprintf(fLdispnm,"%s/LdispM_%.0f_%s_%.1f_%.1f.txt",theta,dirlay,nodeid,lon,lat);
    write_ASC(modelnew,paranew,fRdispnm,fLdispnm,fAZRdispnm,fAZLdispnm,Rsurflag,Lsurflag,0,0,0,0);
    //*/


    }//for theta
    /*
    perturb_para();
    //para2mod();
    Vpara2Lovepara();

    for(i=0;i<para1.Rnpara;i++)printf("i=%d paraR=%g paraL=%g\n",i,para1.Rparameter[i],para1.Lparameter[i]);//--test--
    //updatemodelTibet(model0,depcri1,depcri2,qpcri,qscri);
    //compute_dispMineos(model0,PREM,Nprem,1,1);
    model1=model0;
    //para2mod(para1,model0,model1);
    get_misfitMineos(model1,para1,1,1,depcri1,depcri2,qpcri,qscri,PREM,Nprem);
    //------------------write out model_average--------------------------------------------------

    sprintf(modnm1,"%s/AnimodM_%s_%.1f_%.1f.txt",dirlay,nodeid,lon,lat);
    sprintf(modnm2,"%s/AnimodK_%s_%.1f_%.1f.txt",dirlay,nodeid,lon,lat);
    sprintf(Rdispnm1,"%s/AniRdispM_%s_%.1f_%.1f.txt",dirlay,nodeid,lon,lat);
    sprintf(Rdispnm2,"%s/AniRdispK_%s_%.1f_%.1f.txt",dirlay,nodeid,lon,lat);
    sprintf(Ldispnm1,"%s/AniLdispM_%s_%.1f_%.1f.txt",dirlay,nodeid,lon,lat);
    sprintf(Ldispnm2,"%s/AniLdispK_%s_%.1f_%.1f.txt",dirlay,nodeid,lon,lat);

    write_ASC(model1,para1,modnm1,Rdispnm1,Ldispnm1,1,1);


    vector<vector<vector<double> > >().swap(kernel1);//free
    vector<vector<vector<double> > >().swap(kernel2);//free
    vector<paradef>().swap(paralst);//free
    vector<double>().swap(Rparastd);//free
    vector<double>().swap(Lparastd);//free
  }//while 1
  vector<vector<double> >().swap(PREM);//free
  fclose(fmisfit); 
*/
  }//while1
  return 1;
}
//main
