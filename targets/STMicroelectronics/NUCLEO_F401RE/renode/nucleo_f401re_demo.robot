*** Settings ***
Suite Setup     Setup
Suite Teardown  Teardown
Test Setup      Reset Emulation
Test Teardown   Test Teardown
Resource        ${RENODEKEYWORDS}

*** Test Cases ***
Should Pass Startup Self-Tests
    Execute Command          include @${CURDIR}/nucleo_f401re_demo.resc
    Create Terminal Tester   sysbus.usart2

    Wait For Line On Uart    Eclipse ThreadX Device Monitor Demo                    timeout=15
    Wait For Line On Uart    [SELF-TEST] Starting BSP & Runtime Verification...     timeout=15
    Wait For Line On Uart    [SELF-TEST] All startup verification tests PASSED!     timeout=15

Should Run ThreadX Scheduler And Exercise RTOS Primitives
    Execute Command          include @${CURDIR}/nucleo_f401re_demo.resc
    Create Terminal Tester   sysbus.usart2

    Wait For Line On Uart    [SELF-TEST] All startup verification tests PASSED!     timeout=15
    Wait For Line On Uart    System Status:                                         timeout=15
    # The blink thread drives bsp_led_toggle() on PA5 and the application timer
    # drives the wake counter, so a non-zero pair covers both BSP paths.
    Wait For Line On Uart    Runs: Monitor: (\\d+) .* Blink: [1-9]\\d*              timeout=20  treatAsRegex=true
    Wait For Line On Uart    Mutex Locks: [1-9]\\d*.*Queue Msgs: [1-9]\\d*          timeout=20  treatAsRegex=true
