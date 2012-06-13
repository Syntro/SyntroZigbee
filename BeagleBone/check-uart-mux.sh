#!/bin/sh

UART1_TXD=/sys/kernel/debug/omap_mux/uart1_txd
UART1_RXD=/sys/kernel/debug/omap_mux/uart1_rxd
echo ""
echo "UART1 TX MUX"
cat $UART1_TXD
echo "UART1 RX MUX"
cat $UART1_RXD
echo ""

UART2_TXD=/sys/kernel/debug/omap_mux/spi0_d0
UART2_RXD=/sys/kernel/debug/omap_mux/spi0_sclk
echo "UART2 TX MUX"
cat $UART2_TXD
echo "UART2 RX MUX"
cat $UART2_RXD
echo ""

UART4_TXD=/sys/kernel/debug/omap_mux/gpmc_wpn
UART4_RXD=/sys/kernel/debug/omap_mux/gpmc_wait0
echo "UART4 TX MUX"
cat $UART4_TXD
echo "UART4 RX MUX"
cat $UART4_RXD
echo ""

UART5_TXD=/sys/kernel/debug/omap_mux/lcd_data8
UART5_RXD=/sys/kernel/debug/omap_mux/lcd_data9
echo "UART5 TX MUX"
cat $UART5_TXD
echo "UART5 RX MUX"
cat $UART5_RXD
echo ""
 
