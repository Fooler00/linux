import { appPage } from "./AppPage";
import { FormPage } from "./FormPage";

export class WatchPage extends FormPage {
  async root() {
    return appPage.section("实时监听备份");
  }

  async fill(source: string, destination: string, interval = 1) {
    const root = await this.root();
    await (await this.input(root, "监听源目录")).setValue(source);
    await (await this.input(root, "备份目标目录")).setValue(destination);
    await (await this.input(root, "检测间隔（秒）")).setValue(String(interval));
  }

  async start() {
    const root = await this.root();
    await (await this.button(root, "启动监听")).click();
  }

  async stop() {
    const root = await this.root();
    await (await this.button(root, "停止监听")).click();
  }

  async startButtonDisabled() {
    const root = await this.root();
    return !(await (await this.button(root, "启动监听")).isEnabled());
  }
}

export const watchPage = new WatchPage();
