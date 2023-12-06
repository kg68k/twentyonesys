// printopt
//   (V)TwentyOne.sys function call example by TcbnErik

// Copying and distribution of this file, with or without modification,
// are permitted in any medium without royalty provided the copyright
// notice and this notice are preserved.  This file is offered as-is,
// without any warranty.

#include <stdio.h>
#include <stdlib.h>

#include "twoncall.h"

#define PRINT(flag, mes)             \
  do {                               \
    if (ret.flags.flag) printf(mes); \
  } while (0)

int main(void) {
  union {
    int val;
    TwonFlags flags;
  } ret;
  char sysroot[SYSROOT_MAX];

  if (GetTwentyOneID() != TWON_ID) {
    printf("(V)TwentyOne.sys は登録されていません.\n");
    return EXIT_FAILURE;
  }

  ret.val = GetTwentyOneOptions();

  // オプションを表示する
  PRINT(twentyone_option, "+T 21 文字認識オプション\n");
  PRINT(case_option, "+C ケースオプション\n");
  PRINT(casewarn_option, "+W ケースミスマッチ警告オプション\n");
  PRINT(multi_period_option, "+P ぴりおどたくさんオプション\n");
  PRINT(special_option, "+S 特殊キャラクタオプション\n");
  PRINT(files_option, "+F ファイル検索補完オプション\n");
  PRINT(alias_option, "+A 短縮名オプション\n");
  PRINT(sysroot_option, "+R '/'展開オプション\n");
  PRINT(sysroot2_option, "+Y '\'展開オプション\n");
  PRINT(verbose_option, "+V バーボーズオプション\n");
  PRINT(dummy_option, "未定義のオプションが設定されています。\n");

  // バッファ容量を表示する
  printf("-B=%d\n", ret.flags.buffers_option);

  // SYSROOTを表示する
  GetTwentyOneSysroot(sysroot);
  printf("SYSROOT = %s/\n", sysroot);

  return EXIT_SUCCESS;
}
