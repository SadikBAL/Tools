using Google.Cloud.Firestore;

namespace EventOrganizer.Models;

[FirestoreData]
public class TemplateSlot
{
    [FirestoreProperty("row")]
    public int Row { get; set; }

    [FirestoreProperty("col")]
    public int Col { get; set; }
}
