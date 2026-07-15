import { appPage } from "./AppPage";
import { FormPage } from "./FormPage";

export class SchedulePage extends FormPage {
  async root() {
    return appPage.section("定时备份");
  }

  async fill(source: string, destination: string, interval = 1, maxBackups = 0) {
    const root = await this.root();
    await (await this.input(root, "源目录")).setValue(source);
    await (await this.input(root, "备份目标目录")).setValue(destination);
    await (await this.input(root, "定时间隔（秒）")).setValue(String(interval));
    await (await this.input(root, "最多保留备份数（0=不限）")).setValue(String(maxBackups));
  }

  async start() {
    const root = await this.root();
    await (await this.button(root, "启动定时备份")).click();
  }

  async refresh() {
    const root = await this.root();
    await (await this.button(root, "刷新列表")).click();
  }

  async stopFirst() {
    const root = await this.root();
    await (await this.button(root, "停止")).click();
  }
}

export const schedulePage = new SchedulePage();
