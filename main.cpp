/******************windows桌面背景更换C++程序***********************************************************
功能：获取每日bing搜索主页图片，设置为当日桌面壁纸。并将其下载保存至本地文件夹方便以后浏览
作者：xiaoxi666
日期：2017/03/12

技术要点：
    1、获取网络地址   直接使用网络地址或下载 注意若下载下来后，要将\转换为/,当然也可以用\\
       网络地址可以从这里获取：http://cn.bing.com/HPImageArchive.aspx?format=xml&idx=0&n=1
       在返回  的xml页面中(images->image->url)找到具体的图片地址(xml解析)，拼接到bing域名后面，构成完整地址
       注：xml解析用了TinyXml2

    2、转换图片格式(jpg->bmp)，本程序中的SystemParametersInfoA函数只支持bmp
       在程序中自动转换(单单改后缀名是没有用的)，转换用的程序是从网上下载的，用C语言编写而成
       考虑到需要改后缀名，那就直接下载图片好了，顺便存储之

    3、图片保存路径为C:\Users\Administrator\bingPicture\,格式为.jpg 方便以后浏览
       注意：部分用户电脑可能不存在路径C:/Users/Administrator/,造成程序无法执行，可以直接在C盘根目录下创建路径，如C:/bingPicture/
       注意不保存转换后的bmp格式图片(设置背景后即删除)，因为体积较大
       判断文件夹是否存在，若不存在，则自动创建文件夹

*未实现的功能*：
    ***获取每日壁纸的故事(利用bing故事接口) ，更新壁纸后显示在执行框中

    ***开机自启动，并隐藏到托盘中(为减少CPU占用并增加趣味性，设置为开机自动启动，提示网络连接，并输入"go"才执行功能)
       电脑若未关机，则在24:00自动启动，更换背景

    ***软件自动更新版本功能

******************************************************************************************************/

#include <iostream>    //输入输出
#include <cstring>     //文件命名处理需要用字符串
#include <windows.h>   //调用操作系统各种API
#include <ctime>       //获取时间，各种文件命名
#include <UrlMon.h>    //包含提供下载服务的API
#include "tinyxml2.h"  //解析XML
#include <io.h>        //判断文件夹是否存在
#include <direct.h>    //创建文件夹
extern "C"
{
#include "jpeg.h"    //转换图片格式jpg->bmp  转换格式的程序使用C语言写的
}

//创建本地bingPicture路径和Tmp路径
void createDir()
{
    //本地bingPicture路径
    std::string LocalFolder="C:/Users/Administrator/bingPicture/";

    if(0!=access(LocalFolder.c_str(),0))    //判断文件夹是否存在，若不存在则创建
        if(0!=mkdir(LocalFolder.c_str()))
            std::cout<<"创建文件夹bingPicture失败！"<<std::endl;
        else
            std::cout<<"创建文件夹bingPicture成功！"<<std::endl;
    else
        std::cout<<"文件夹bingPicture已存在！"<<std::endl;

    //本地Tmp路径
    std::string LocalXmlFolder="C:/Users/Administrator/bingPicture/Tmp/";

    if(0!=access(LocalXmlFolder.c_str(),0))    //判断文件夹是否存在，若不存在则创建
        if(0!=mkdir(LocalXmlFolder.c_str()))
            std::cout<<"创建临时文件夹Tmp失败！"<<std::endl;
        else
            std::cout<<"创建临时文件夹Tmp成功！"<<std::endl;
    else
        std::cout<<"临时文件夹Tmp已存在！"<<std::endl;

}

/**************************************************************************************
首先明白一个概念，即string替换所有字符串，将"12212"这个字符串的所有"12"都替换成"21"，结果是什么？
可以是22211，也可以是21221，有时候应用的场景不同，就会希望得到不同的结果，所以这两种答案都做了实现。
**************************************************************************************/
//替换字符串方法1(完全轮询，替换一次后接着再次扫描，因为替换一次后可能又出现了满足替换条件的字符串)
std::string & replace_all(std::string& str,const std::string& old_value,const std::string& new_value)
{
    while(true)   {
        std::string::size_type pos(0);
        if((pos=str.find(old_value))!=std::string::npos)
            str.replace(pos,old_value.length(),new_value);
        else
            break;
    }
    return str;
}

//替换字符串方法2(只替换一次) 本项目中，只替换\为/用方法2即可
std::string & replace_all_distinct(std::string& str,const std::string& old_value,const std::string& new_value)
{
    for(std::string::size_type pos(0);  pos!=std::string::npos; pos+=new_value.length())
    {
        if((pos=str.find(old_value,pos))!=std::string::npos)
            str.replace(pos,old_value.length(),new_value);
        else   break;
    }
    return str;
}

//获取年月日(命名用)
std::string getYearMonthDay()
{
    time_t timer;
    time(&timer);
    tm* t_tm = localtime(&timer);

    std::string Year=std::to_string(t_tm->tm_year+1900);
    std::string Month=std::to_string(t_tm->tm_mon+1);
    std::string Day=std::to_string(t_tm->tm_mday);
    std::string PictureName=Year+"_"+Month+"_"+Day;

    return PictureName;
}

//获取图片的Xml并解析图片的url路径
std::string getPicTureXmlAndUrl()
{
    //网络上的XML路径
    std::string WebXmlpath ="http://cn.bing.com/HPImageArchive.aspx?format=xml&idx=0&n=1";
    //本地Xml路径
    std::string LocalXmlFolder="C:/Users/Administrator/bingPicture/Tmp/";
    std::string LocalXmleach=getYearMonthDay();
    std::string LocalXmlFullpath=LocalXmlFolder+LocalXmleach+".xml";

    if(URLDownloadToFileA(NULL,
                         WebXmlpath.c_str(),
                         LocalXmlFullpath.c_str(),
                         0,
                         NULL)
            ==S_OK)
    {
        std::cout<<"Xml下载成功！即将解析今日壁纸Url！"<<std::endl;

        /***************下面开始解析xml中的url路径*******************/
        tinyxml2::XMLDocument doc;
        if(tinyxml2::XML_SUCCESS != doc.LoadFile(LocalXmlFullpath.c_str()))
            std::cout<<"读取Xml文件异常！"<<std::endl;
        tinyxml2::XMLElement *images=doc.RootElement();
        tinyxml2::XMLElement *image =images->FirstChildElement("image");

        //图片Url
        std::string WebPicturedomain="http://cn.bing.com";
        std::string WebPictureUrl="";

        if(image!=NULL)
            WebPictureUrl=image->FirstChildElement("url")->GetText();

        std::string WebPictureFullpath =WebPicturedomain+WebPictureUrl;
        std::cout<<"今日壁纸Url解析成功！地址为："<<std::endl;
        std::cout<<WebPictureFullpath<<std::endl;
        /*********************************************************/
        return WebPictureFullpath;
    }
    else
    {
        std::cout<<"Xml下载失败！无法获取图片Url！请检查网络连接是否正常！"<<std::endl;
        return "error";
    }

}

//从网络上下载图片并存储到本地
std::string getPicture(std::string WebFullpath)
{
    //本地存储路径
    std::string LocalFolder="C:/Users/Administrator/bingPicture/";
    std::string Localeach=getYearMonthDay();
    std::string LocalFullpath=LocalFolder+Localeach+".jpg";

    if(URLDownloadToFileA(NULL,
                         WebFullpath.c_str(),
                         LocalFullpath.c_str(),
                         0,
                         NULL)
            ==S_OK)
    {
        std::cout<<"今日壁纸下载成功！"<<std::endl;

        /***************下面转换图片格式jpg->bmp******************/
        //临时文件夹Tmp路径
        std::string TmpFolder="C:/Users/Administrator/bingPicture/Tmp/";
        //.bmp图片路径
        std::string bmpFolder=TmpFolder+getYearMonthDay()+".bmp";
        LoadJpegFile(const_cast<char *>(LocalFullpath.c_str()),const_cast<char *>(bmpFolder.c_str()));
        /*******************************************************/
        return bmpFolder;
    }
    else
    {
        std::cout<<"壁纸下载失败！请检查网络连接是否正常！"<<std::endl;
        return "error";
    }
}

//改变桌面背景成功后，删除bmp文件和xml文件(只保留jpg文件)，此步骤需要小心，避免删除错误路径下的内容
void deleteBmpAndXml()
{
    //临时文件夹Tmp路径
    std::string TmpFolder="C:/Users/Administrator/bingPicture/Tmp/";
    //.bmp图片路径
    std::string bmpFolder=TmpFolder+getYearMonthDay()+".bmp";
    //xml文件路径
    std::string xmlFolder=TmpFolder+getYearMonthDay()+".xml";

    if(0==access("C:/Users/Administrator/bingPicture/Tmp/",0))    //判断文件夹是否存在，若存在则删除
    {
        //删除bmp图片
        if(0==access(bmpFolder.c_str(),0))
        {
            if(0==remove(bmpFolder.c_str()))
                std::cout<<"删除临时bmp格式图片成功！"<<std::endl;
            else
                std::cout<<"删除临时bmp格式图片失败！"<<std::endl;
        }
        else
            std::cout<<"临时bmp格式图片不存在！"<<std::endl;

        //删除xml文件
        if(0==access(xmlFolder.c_str(),0))
        {
            if(0==remove(xmlFolder.c_str()))
                std::cout<<"删除xml文件成功！"<<std::endl;
            else
                std::cout<<"删除xml文件失败！"<<std::endl;
        }
        else
            std::cout<<"xml文件不存在！"<<std::endl;

        //删除Tmp文件夹(注意此函数只能删除空文件夹,因此要先删除文件夹中的文件)
        if(0==rmdir(TmpFolder.c_str()))
            std::cout<<"临时文件夹Tmp已删除！"<<std::endl;
        else
            std::cout<<"临时文件夹Tmp删除失败！"<<std::endl;
    }
    else
        std::cout<<"临时文件夹Tmp不存在！"<<std::endl;

}

//改变桌面背景(PictureFullpath:图片完整路径)
void changePicture(std::string PictureFullpath)
{
    bool result=false;
    result=SystemParametersInfoA(SPI_SETDESKWALLPAPER,
                          0,
                          (PVOID)PictureFullpath.c_str(),
                          0);
    if(result==false)
    {
        std::cout<<"今日壁纸更新失败！请联系开发人员！"<<std::endl;
    }

    else
    {
        SystemParametersInfoA(SPI_SETDESKWALLPAPER,
                                  0,
                                  (PVOID)PictureFullpath.c_str(),
                                  SPIF_SENDCHANGE);
        deleteBmpAndXml();
        system("cls");
        std::cout<<"version:1.0.0(小包砸专版)"<<std::endl<<std::endl;
        std::cout<<"今日壁纸更新成功！"<<std::endl<<std::endl;
        std::cout<<"美好的一天开始啦！用心享受吧！"<<std::endl<<std::endl;
    }
}

int main()
{
    std::string startOrder="";
    std::cout<<"嗨！小包砸！你的贴心壁纸小助手已启动！将为你设置今日壁纸哦！"<<std::endl<<std::endl;
    std::cout<<"请确保电脑网络连接状况良好,准备好后输入go"<<std::endl<<std::endl;
    std::cout<<"小包砸的指令： ";
    std::cin>>startOrder;
    while("go"!=startOrder)
    {
        std::cout<<"哎呀输错了呢,重新输入吧： ";
        std::cin>>startOrder;
    }
    if("go"==startOrder)
    {
        createDir();
        changePicture(getPicture(getPicTureXmlAndUrl()));
    }

    /*******************************以下为个性化字幕输出，与程序核心功能无关************************/
    std::string umua0="          ***      ***   **************   ***      ***   ***********   ";
    std::string umua1="          ***      ***   **************   ***      ***   ***********   ";
    std::string umua2="          ***      ***   ***   **   ***   ***      ***   ***     ***   ";
    std::string umua3="          ***      ***   ***   **   ***   ***      ***   ***     ***   ";
    std::string umua4="          ***      ***   ***   **   ***   ***      ***   ***********   ";
    std::string umua5="          ***      ***   ***   **   ***   ***      ***   ***     ***   ";
    std::string umua6="          ***      ***   ***   **   ***   ***      ***   ***     ***   ";
    std::string umua7="          ***      ***   ***   **   ***   ***      ***   ***     ***   ";
    std::string umua8="          ************   ***   **   ***   ************   ***     ***   ";
    std::string umua9="          ************   ***   **   ***   ************   ***     ***   ";

    #define mua(n) std::cout<<umua##n<<std::endl;
    mua(0);mua(1);mua(2);mua(3);mua(4);mua(5);mua(6);mua(7);mua(8);mua(9);
    std::cout<<std::endl<<std::endl<<"按回车键即可退出本程序哦！"<<std::endl<<std::endl;
    /******************************************************************************************/
    return 0;
}
