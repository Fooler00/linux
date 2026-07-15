import { appPage } from "./AppPage";
import { FormPage } from "./FormPage";

export class TaskListPage extends FormPage {
  async root() {
    return appPage.section("任务列表");
  }

  async refresh() {
    const root = await this.root();
    await (await this.button(root, "手动刷新")).click();
  }
}

export const taskListPage = new TaskListPage();
