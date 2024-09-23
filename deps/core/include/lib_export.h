#ifdef MYLIBARAY_EXPORTS					//  add by cmake
	#define MYLIB_API __declspec(dllexport) //  导出
#else
	#define MYLIB_API __declspec(dllimport) //  导入
#endif