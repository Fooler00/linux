import { appPage } from "./AppPage";
import { FormPage } from "./FormPage";

export class RestorePage extends FormPage {
  async root() {
    return appPage.section("还原备份");
  }

  async fill(backupPath: string, destination: string, password = "") {
    const root = await this.root();
    await (await this.input(root, "备份文件/目录路径")).setValue(backupPath);
    await (await this.input(root, "还原目标目录")).setValue(destination);
    if (password) {
      await (await this.input(root, "解密密码（加密备份需要）")).setValue(password);
    }
  }

  async submit() {
    const root = await this.root();
    await (await this.button(root, "开始还原")).click();
  }
}

export const restorePage = new RestorePage();
