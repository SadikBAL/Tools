using Google.Cloud.Firestore;

namespace EventOrganizer.Models;

[FirestoreData]
public class Participant
{
    [FirestoreProperty("userId")]
    public string UserId { get; set; } = "";

    [FirestoreProperty("pictureUrl")]
    public string PictureUrl { get; set; } = "";

    [FirestoreProperty("name")]
    public string Name { get; set; } = "";

    [FirestoreProperty("slotIndex")]
    public int? SlotIndex { get; set; }
}
