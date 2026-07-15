import { appPage } from "./AppPage";
import { FormPage } from "./FormPage";

export class CloudPage extends FormPage {
  async root() {
    return appPage.section("云存储");
  }

  async setRemoteDir(remoteDir: string) {
    const root = await this.root();
    await (await this.input(root, "云端目录")).setValue(remoteDir);
  }

  async refresh() {
    const root = await this.root();
    await (await this.button(root, "刷新列表")).click();
  }

  async fillUpload(localPath: string, remotePath: string) {
    const root = await this.root();
    await (await this.input(root, "本地文件")).setValue(localPath);
    await (await this.input(root, "云端保存路径")).setValue(remotePath);
  }

  async upload() {
    const root = await this.root();
    await (await this.button(root, "上传到云端")).click();
  }

  async deleteFirst() {
    await browser.execute(() => {
      window.confirm = () => true;
    });
    const root = await this.root();
    await (await this.button(root, "删除")).click();
  }
}

export const cloudPage = new CloudPage();
