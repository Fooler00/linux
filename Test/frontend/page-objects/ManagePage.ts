import { appPage } from "./AppPage";
import { FormPage } from "./FormPage";

export class ManagePage extends FormPage {
  async root() {
    return appPage.section("备份管理与淘汰");
  }

  async setDestination(destination: string) {
    const root = await this.root();
    await (await this.input(root, "备份目标目录")).setValue(destination);
  }

  async setMaxBackups(value: number) {
    const root = await this.root();
    await (await this.input(root, "最多保留数量（0=不限）")).setValue(String(value));
  }

  async query() {
    const root = await this.root();
    await (await this.button(root, "查询备份")).click();
  }

  async prune() {
    const root = await this.root();
    await (await this.button(root, "执行淘汰")).click();
  }

  async viewMetadata() {
    const root = await this.root();
    await (await this.button(root, "查看元数据")).click();
  }
}

export const managePage = new ManagePage();
