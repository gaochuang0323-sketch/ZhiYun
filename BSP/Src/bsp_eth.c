#include "bsp_eth.h"

void BSP_ETH_PHY_Reset(void)
{
#if defined(ETH_nRST_Pin) && defined(ETH_nRST_GPIO_Port)
  HAL_GPIO_WritePin(ETH_nRST_GPIO_Port, ETH_nRST_Pin, GPIO_PIN_RESET);
  HAL_Delay(200U);
  HAL_GPIO_WritePin(ETH_nRST_GPIO_Port, ETH_nRST_Pin, GPIO_PIN_SET);
  HAL_Delay(300U);
#else
  /*
   * Current EIS-Replayer hardware pulls ETH_nRST high and does not connect it
   * to the MCU. PHY reset is therefore handled by hardware power-on reset only.
   */
#endif
}
