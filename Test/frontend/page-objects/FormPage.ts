export class FormPage {
  protected async byLabel(root: any, label: string, tag = "*") {
    const field = await root.$(
      `.//label[.//span[normalize-space()="${label}"]]//${tag}[self::input or self::textarea or self::select]`
    );
    await field.waitForExist({ timeout: 10000 });
    return field;
  }

  protected async input(root: any, label: string) {
    return this.byLabel(root, label, "input");
  }

  protected async textarea(root: any, label: string) {
    return this.byLabel(root, label, "textarea");
  }

  protected async select(root: any, label: string) {
    return this.byLabel(root, label, "select");
  }

  protected async button(root: any, label: string) {
    const button = await root.$(`.//button[contains(normalize-space(), "${label}")]`);
    await button.waitForClickable({ timeout: 10000 });
    return button;
  }
}
