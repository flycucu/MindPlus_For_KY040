//% color="#9C27B0" iconWidth=50 iconHeight=40
namespace cucuky040 {

    function addBase() {
        Generator.addInclude('cucuky040', '#include <MindPlusKY040.h>');
        let defPins = 'myEncoder(2, 3, 4);';
        if (Generator.board === 'esp32' || Generator.board === 'firebeetleesp32' ||
            Generator.board === 'firebeetleesp32e' || Generator.board === 'telloesp32' ||
            Generator.board === 'unihiker' || Generator.board === 'handbit') {
            defPins = 'myEncoder(27, 26, 25);';
        } else if (Generator.board === 'esp32s3bit') {
            defPins = 'myEncoder(8, 9, 10);';
        } else if (Generator.board === 'esp8266') {
            defPins = 'myEncoder(14, 12, 5);';
        } else if (Generator.board === 'microbit' || Generator.board === 'maqueen' ||
                   Generator.board === 'calliope') {
            defPins = 'myEncoder(0, 1, 2);';
        } else if (Generator.board === 'pico') {
            defPins = 'myEncoder(2, 3, 4);';
        } else if (Generator.board === 'maixduino') {
            defPins = 'myEncoder(8, 9, 10);';
        }
        Generator.addObject('myEncoder', 'MindPlusKY040', defPins);
    }

    //% block="初始化 KY-040 CLK[CLK] DT[DT] SW[SW]" blockType="command"
    //% CLK.shadow="dropdown" CLK.options="KY040_CLK" CLK.defl="default_begin_CLK"
    //% DT.shadow="dropdown" DT.options="KY040_DT" DT.defl="default_begin_DT"
    //% SW.shadow="dropdown" SW.options="KY040_SW" SW.defl="default_begin_SW"
    export function begin(parameter: any, block: any) {
        let clk = parameter.CLK.code;
        let dt = parameter.DT.code;
        let sw = parameter.SW.code;
        Generator.addInclude('cucuky040', '#include <MindPlusKY040.h>');
        Generator.addObject('myEncoder', 'MindPlusKY040', `myEncoder(${clk}, ${dt}, ${sw});`, true);
        Generator.addSetup('myEncoder.begin', 'myEncoder.begin();');
    }

    //% block="---"
    export function noteSep1() {}

    //% block="编码器变换" blockType="reporter"
    export function getChange(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.getChange()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="编码器计数值" blockType="reporter"
    export function getCount(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.getCount()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="设置编码器计数值为[COUNT]" blockType="command"
    //% COUNT.shadow="number" COUNT.defl=0
    export function setCount(parameter: any, block: any) {
        let count = parameter.COUNT.code;
        addBase();
        Generator.addCode(`myEncoder.setCount(${count});`);
    }

    //% block="编码器计数归零" blockType="command"
    export function resetCount(parameter: any, block: any) {
        addBase();
        Generator.addCode('myEncoder.resetCount();');
    }

    //% block="本次转动步数" blockType="reporter"
    export function getDelta(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.getDelta()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="编码器有转动?" blockType="boolean"
    export function hasMoved(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.hasMoved()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="顺时针转动?" blockType="boolean"
    export function turnedClockwise(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.turnedClockwise()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="逆时针转动?" blockType="boolean"
    export function turnedCounterClockwise(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.turnedCounterClockwise()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="---"
    export function noteSep2() {}

    //% block="编码器按键按下?" blockType="boolean"
    export function isButtonPressed(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.isButtonPressed()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="编码器按键被单击?" blockType="boolean"
    export function wasButtonClicked(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.wasButtonClicked()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="编码器按键被松开?" blockType="boolean"
    export function wasButtonReleased(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.wasButtonReleased()', Generator.ORDER_UNARY_POSTFIX]);
    }
}
