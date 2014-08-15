@echo off
call take_pic.bat "将拍摄Black Level图片1，请确保camera保持全黑状态" "BLACK_LEVEL1"
call take_pic.bat "将拍摄Black Level图片2，请确保camera保持全黑状态" "BLACK_LEVEL2"
call take_pic.bat "将拍摄Black Level图片3，请确保camera保持全黑状态" "BLACK_LEVEL3"

@echo "请将曝光调到+1，同时打开反带,亮度大于等于1000lux"
call take_pic.bat "A" "ROLLOFF_1000_A"
call take_pic.bat "D65" "ROLLOFF_1000_D65"
call take_pic.bat "TL84" "ROLLOFF_1000_TL84"
@echo "请将曝光调到+1，同时打开反带,亮度小于等于20lux"
call take_pic.bat "20LUX A" "ROLLOFF_20_A"
call take_pic.bat "20LUX D65" "ROLLOFF_20_D65"
call take_pic.bat "20LUX TL84" "ROLLOFF_20_TL84"

@echo "请将曝光调到0，同时打开反带"


@echo "下面将拍摄MCC，MCC 70/100FOV"
call take_pic.bat "D65 1000LUX" "MCC_MCC_1000_D65"
call take_pic.bat "D65 10LUX" "MCC_MCC_10_D65"
call take_pic.bat "A 1000LUX" "MCC_MCC_1000_A"
call take_pic.bat "A 10LUX" "MCC_MCC_10_A"
call take_pic.bat "D50" "MCC_MCC_1000_D50"
call take_pic.bat "CWF" "MCC_MCC_1000_CWF"
call take_pic.bat "TL84 1000lux" "MCC_MCC_1000_TL84"
call take_pic.bat "TL84 500lux" "MCC_MCC_500_TL84"
call take_pic.bat "TL84 200lux" "MCC_MCC_200_TL84"
call take_pic.bat "TL84 100lux" "MCC_MCC_100_TL84"
call take_pic.bat "TL84 50lux" "MCC_MCC_50_TL84"
call take_pic.bat "TL84 10lux" "MCC_MCC_10_TL84"
call take_pic.bat "Outdoor 11am-3pm" "MCC_MCC_outdoor_11am_3pm"

@echo "下面将拍摄Macbetch chart ff 1000LUX"
call take_pic.bat "A" "MCC_FF_1000_A"
call take_pic.bat "D65" "MCC_FF_1000_D65"
call take_pic.bat "TL84" "MCC_FF_1000_TL84"
call take_pic.bat "TL84 20lux" "MCC_FF_20_TL84"



@echo "下面将拍摄AWB Reference Points,18/100 gray card,100/100FOV ,1000LUX"
call take_pic.bat "D75" "AWB_REF_D75"
call take_pic.bat "D65" "AWB_REF_D65"
call take_pic.bat "D50" "AWB_REF_D50"
call take_pic.bat "TL84 1000lux" "AWB_REF_TL84_1000"
call take_pic.bat "TL84 500lux" "AWB_REF_TL84_500"
call take_pic.bat "TL84 200lux" "AWB_REF_TL84_200"
call take_pic.bat "TL84 100lux" "AWB_REF_TL84_100"
call take_pic.bat "TL84 50lux" "AWB_REF_TL84_50"
call take_pic.bat "TL84 10lux" "AWB_REF_TL84_10"
call take_pic.bat "CWF" "AWB_REF_CWF"
call take_pic.bat "A" "AWB_REF_A"
call take_pic.bat "H" "AWB_REF_H"
call take_pic.bat "Outdoor" "AWB_REF_Outdoor"
call take_pic.bat "U30" "AWB_REF_U30"

@echo "下面将拍摄ASF,TL84 ISO12233 Chart 4:3"

call take_pic.bat "TL84 1000lux" "ASF_TL84_1000"
call take_pic.bat "TL84 200lux" "ASF_TL84_200"
call take_pic.bat "TL84 10lux" "ASF_TL84_10"

@echo "全部拍摄完毕"
