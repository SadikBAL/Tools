using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;

namespace UnrealProjectCleaner;

public partial class Main : Form
{
    private Dictionary<string, int> _folders;
    public Main()
    {
        _folders = new Dictionary<string, int>();
        InitializeComponent();
    }

    private void Main_Load(object sender, EventArgs e)
    {
        UpdateCheckedListBoxValues();
    }
    public static Dictionary<string, int> FindBinariesAndIntermediateFoldersWithCount()
    {
        Dictionary<string, int> fileAndFileCount = new Dictionary<string, int>();
        string startPath = AppDomain.CurrentDomain.BaseDirectory;
        var foundPaths = new List<string>();
        try
        {
            string[] allDirectories = Directory.GetDirectories(startPath, "*", SearchOption.AllDirectories);

            foreach (string dirPath in allDirectories)
            {
                string dirName = Path.GetFileName(dirPath);
                if (dirName.Equals("Binaries", StringComparison.OrdinalIgnoreCase) ||
                    dirName.Equals("Intermediate", StringComparison.OrdinalIgnoreCase))
                {
                    foundPaths.Add(dirPath);
                    fileAndFileCount.Add(dirPath, Directory.GetFiles(dirPath, "*", SearchOption.AllDirectories).Length);
                }
            }
        }
        catch (UnauthorizedAccessException ex)
        {
            Console.WriteLine($"Erişim hatası: Bir veya daha fazla klasöre erişim yetkisi yok. {ex.Message}");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Beklenmedik bir hata oluştu: {ex.Message}");
        }

        return fileAndFileCount;
    }
    private void buttonCheckedAll_Click(object sender, EventArgs e)
    {
        for (int i = 0; i < checkedDirectories.Items.Count; i++)
        {
            checkedDirectories.SetItemChecked(i, true);
        }
    }
    private void buttonUncheckedAll_Click(object sender, EventArgs e)
    {
        for (int i = 0; i < checkedDirectories.Items.Count; i++)
        {
            checkedDirectories.SetItemChecked(i, false);
        }
    }
    private void buttonClean_Click(object sender, EventArgs e)
    {
        for (int i = 0; i < checkedDirectories.Items.Count; i++)
        {
            if (checkedDirectories.GetItemChecked(i))
            {
                string[] allDirectories = Directory.GetDirectories(_folders.ElementAt(i).Key, "*", SearchOption.AllDirectories);
                foreach (string directory in allDirectories)
                {
                    DeleteFiles(directory);
                }

            }
        }
        UpdateCheckedListBoxValues();
        string startPath = AppDomain.CurrentDomain.BaseDirectory;
        string[] projectFile = Directory.GetFiles(startPath, "*.uproject");
        if (projectFile.Length > 0)
        {
            try
            {
                GenerateProjectFiles(projectFile[0]);
            }
            catch
            {

            }
        }
       
    }
    public void DeleteFiles(string dizinYolu)
    {
        try
        {
            if (!Directory.Exists(dizinYolu))
            {
                return;
            }
            string[] dosyaYollari = Directory.GetFiles(dizinYolu + "\\");
            foreach (string dosyaYolu in dosyaYollari)
            {
                File.Delete(dosyaYolu);
            }
            string startPath = AppDomain.CurrentDomain.BaseDirectory;
            string[] slnDosyalari = Directory.GetFiles(startPath, "*.sln");
            if (slnDosyalari.Length > 0)
            {
                foreach (string dosyaYolu in slnDosyalari)
                {
                    string dosyaAdi = Path.GetFileName(dosyaYolu);
                    try
                    {
                        File.Delete(dosyaYolu);
                    }
                    catch
                    {
                    }
                }
            }          
        }
        catch (UnauthorizedAccessException)
        {
           MessageBox.Show($"Hata: '{dizinYolu}' dizinine veya içindeki bir dosyaya erişim yetkisi yok.");
        }
        catch (IOException ex)
        {
            MessageBox.Show($"Hata: Bir G/Ç hatası oluştu. Dosyalardan biri kullanımda olabilir. Detay: {ex.Message}");
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Beklenmedik bir hata oluştu: {ex.Message}");
        }
    }
    public void UpdateCheckedListBoxValues()
    {
        checkedDirectories.Items.Clear();
        _folders.Clear();
        _folders = FindBinariesAndIntermediateFoldersWithCount();
        int counter = 0;
        foreach (var pair in _folders)
        {

            counter++;
            if (pair.Value > 0)
            {
                checkedDirectories.Items.Add(pair.Key.ToString() + "[ File Count : " + pair.Value + " ]", counter < 3);
            }
            else
            {
                checkedDirectories.Items.Add(pair.Key.ToString() + "[ File Count : CLEAR ]", counter < 3);
            }
        }
    }
    private void buttonCleanBuild_Click(object sender, EventArgs e)
    {
        buttonClean_Click(sender, e);
        string startPath = AppDomain.CurrentDomain.BaseDirectory;
        string[] projectFile = Directory.GetFiles(startPath, "*.uproject");
        if(projectFile.Length > 0)
        {
            try
            {
                StartAppWithName(projectFile[0]);
            }
            catch
            {

            }
        }
       

    }
    public void StartAppWithName(string uprojectPath)
    {
        try
        {
            if (File.Exists(uprojectPath))
            {
                Process.Start(new ProcessStartInfo(uprojectPath) { UseShellExecute = true });
            }
            else
            {
                MessageBox.Show($"Dosya bulunamadı: {uprojectPath}", "Hata", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Dosya açılırken bir hata oluştu: {ex.Message}", "Hata", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
    public void GenerateProjectFiles(string uprojectDosyaYolu)
    {
        string selectorPath = @"C:\Program Files (x86)\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe";
        if (!File.Exists(selectorPath))
        {
            System.Windows.Forms.MessageBox.Show("UnrealVersionSelector.exe bulunamadı!");
            return;
        }

        try
        {
            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.FileName = selectorPath;
            startInfo.Arguments = $"/projectfiles \"{uprojectDosyaYolu}\"";

            startInfo.UseShellExecute = false;
            startInfo.CreateNoWindow = true;
            using (Process process = Process.Start(startInfo))
            {
                process.WaitForExit();
            }
        }
        catch (Exception ex)
        {
            System.Windows.Forms.MessageBox.Show("Proje dosyaları olusturulamadi. Hata: " + ex.Message);
        }
    }
}