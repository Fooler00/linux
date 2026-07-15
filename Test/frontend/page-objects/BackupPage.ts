import { appPage } from "./AppPage";
import { FormPage } from "./FormPage";

export class BackupPage extends FormPage {
  async root() {
    return appPage.section("手动备份");
  }

  async fillPaths(source: string, destination: string) {
    const root = await this.root();
    await (await this.input(root, "源路径（文件或目录）")).setValue(source);
    await (await this.input(root, "备份目标目录")).setValue(destination);
  }

  async setArchive(type: string) {
    const root = await this.root();
    await (await this.select(root, "归档类型")).selectByAttribute("value", type);
  }

  async setEncrypt(password: string) {
    const root = await this.root();
    await (await root.$('.//label[contains(normalize-space(), "启用加密")]//input')).click();
    await (await this.input(root, "加密密码")).setValue(password);
  }

  async openAdvancedFilter() {
    const root = await this.root();
    const summary = await root.$(".//summary[normalize-space()='高级筛选']");
    await summary.click();
  }

  async setExtensions(value: string) {
    const root = await this.root();
    await (await this.input(root, "扩展名")).setValue(value);
  }

  async setExcludePaths(value: string) {
    const root = await this.root();
    await (await this.textarea(root, "排除路径")).setValue(value);
  }

  async submit() {
    const root = await this.root();
    await (await this.button(root, "开始备份")).click();
  }
}

export const backupPage = new BackupPage();
