namespace UnrealProjectCleaner;

partial class Main
{
    /// <summary>
    ///  Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    ///  Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
        if (disposing && (components != null))
        {
            components.Dispose();
        }

        base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    ///  Required method for Designer support - do not modify
    ///  the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
        checkedDirectories = new CheckedListBox();
        buttonCheckedAll = new Button();
        buttonUncheckedAll = new Button();
        buttonCleanBuild = new Button();
        buttonClean = new Button();
        SuspendLayout();
        // 
        // checkedDirectories
        // 
        checkedDirectories.FormattingEnabled = true;
        checkedDirectories.Items.AddRange(new object[] { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" });
        checkedDirectories.Location = new Point(14, 16);
        checkedDirectories.Margin = new Padding(3, 4, 3, 4);
        checkedDirectories.Name = "checkedDirectories";
        checkedDirectories.Size = new Size(1025, 554);
        checkedDirectories.TabIndex = 0;
        // 
        // buttonCheckedAll
        // 
        buttonCheckedAll.Location = new Point(14, 581);
        buttonCheckedAll.Margin = new Padding(3, 4, 3, 4);
        buttonCheckedAll.Name = "buttonCheckedAll";
        buttonCheckedAll.Size = new Size(129, 59);
        buttonCheckedAll.TabIndex = 1;
        buttonCheckedAll.Text = "Checked All";
        buttonCheckedAll.UseVisualStyleBackColor = true;
        buttonCheckedAll.Click += buttonCheckedAll_Click;
        // 
        // buttonUncheckedAll
        // 
        buttonUncheckedAll.Location = new Point(150, 581);
        buttonUncheckedAll.Margin = new Padding(3, 4, 3, 4);
        buttonUncheckedAll.Name = "buttonUncheckedAll";
        buttonUncheckedAll.Size = new Size(129, 59);
        buttonUncheckedAll.TabIndex = 1;
        buttonUncheckedAll.Text = "Unchecked All";
        buttonUncheckedAll.UseVisualStyleBackColor = true;
        buttonUncheckedAll.Click += buttonUncheckedAll_Click;
        // 
        // buttonCleanBuild
        // 
        buttonCleanBuild.Location = new Point(910, 581);
        buttonCleanBuild.Margin = new Padding(3, 4, 3, 4);
        buttonCleanBuild.Name = "buttonCleanBuild";
        buttonCleanBuild.Size = new Size(129, 59);
        buttonCleanBuild.TabIndex = 1;
        buttonCleanBuild.Text = "Clean Build";
        buttonCleanBuild.UseVisualStyleBackColor = true;
        buttonCleanBuild.Click += buttonCleanBuild_Click;
        // 
        // buttonClean
        // 
        buttonClean.Location = new Point(775, 582);
        buttonClean.Margin = new Padding(3, 4, 3, 4);
        buttonClean.Name = "buttonClean";
        buttonClean.Size = new Size(129, 59);
        buttonClean.TabIndex = 1;
        buttonClean.Text = "Clean";
        buttonClean.UseVisualStyleBackColor = true;
        buttonClean.Click += buttonClean_Click;
        // 
        // Main
        // 
        AutoScaleDimensions = new SizeF(8F, 20F);
        AutoScaleMode = AutoScaleMode.Font;
        ClientSize = new Size(1053, 656);
        Controls.Add(buttonClean);
        Controls.Add(buttonCleanBuild);
        Controls.Add(buttonUncheckedAll);
        Controls.Add(buttonCheckedAll);
        Controls.Add(checkedDirectories);
        Margin = new Padding(3, 4, 3, 4);
        Name = "Main";
        Text = "Cleaner";
        Load += Main_Load;
        ResumeLayout(false);
    }

    #endregion

    private CheckedListBox checkedDirectories;
    private Button buttonCheckedAll;
    private Button buttonUncheckedAll;
    private Button buttonCleanBuild;
    private Button buttonClean;
}