//% color="#9C27B0" iconWidth=50 iconHeight=40
namespace cucuky040 {

    function addBase() {
        Generator.addImport('from MindPlusKY040 import MindPlusKY040');
        Generator.addDeclaration('myEncoder', 'myEncoder = MindPlusKY040(8, 9, 10)');
    }

    //% block="初始化 KY-040 CLK[CLK] DT[DT] SW[SW]" blockType="command"
    //% CLK.shadow="dropdown" CLK.options="KY040_CLK" CLK.defl="default_begin_CLK"
    //% DT.shadow="dropdown" DT.options="KY040_DT" DT.defl="default_begin_DT"
    //% SW.shadow="dropdown" SW.options="KY040_SW" SW.defl="default_begin_SW"
    export function begin(parameter: any, block: any) {
        let clk = parameter.CLK.code;
        let dt = parameter.DT.code;
        let sw = parameter.SW.code;
        Generator.addImport('from MindPlusKY040 import MindPlusKY040');
        Generator.addDeclaration('myEncoder', `myEncoder = MindPlusKY040(${clk}, ${dt}, ${sw})`, true);
        Generator.addInit('myEncoder.begin', 'myEncoder.begin()');
    }

    //% block="---"
    export function noteSep1() {}

    //% block="编码器变换" blockType="reporter"
    export function getChange(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.get_change()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="编码器计数值" blockType="reporter"
    export function getCount(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.get_count()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="设置编码器计数值为[COUNT]" blockType="command"
    //% COUNT.shadow="number" COUNT.defl=0
    export function setCount(parameter: any, block: any) {
        let count = parameter.COUNT.code;
        addBase();
        Generator.addCode(`myEncoder.set_count(${count})`);
    }

    //% block="编码器计数归零" blockType="command"
    export function resetCount(parameter: any, block: any) {
        addBase();
        Generator.addCode('myEncoder.reset_count()');
    }

    //% block="本次转动步数" blockType="reporter"
    export function getDelta(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.get_delta()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="编码器有转动?" blockType="boolean"
    export function hasMoved(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.has_moved()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="顺时针转动?" blockType="boolean"
    export function turnedClockwise(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.turned_clockwise()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="逆时针转动?" blockType="boolean"
    export function turnedCounterClockwise(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.turned_counter_clockwise()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="---"
    export function noteSep2() {}

    //% block="编码器按键按下?" blockType="boolean"
    export function isButtonPressed(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.is_button_pressed()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="编码器按键被单击?" blockType="boolean"
    export function wasButtonClicked(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.was_button_clicked()', Generator.ORDER_UNARY_POSTFIX]);
    }

    //% block="编码器按键被松开?" blockType="boolean"
    export function wasButtonReleased(parameter: any, block: any) {
        addBase();
        Generator.addCode(['myEncoder.was_button_released()', Generator.ORDER_UNARY_POSTFIX]);
    }
}
