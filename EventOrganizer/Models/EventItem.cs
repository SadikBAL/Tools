using Google.Cloud.Firestore;

namespace EventOrganizer.Models;

[FirestoreData]
public class EventItem
{
    [FirestoreDocumentId]
    public string Id { get; set; } = "";

    [FirestoreProperty("title")]
    public string Title { get; set; } = "";

    [FirestoreProperty("category")]
    public string Category { get; set; } = ""; // music | tech | art | sport

    [FirestoreProperty("date")]
    public DateTime Date { get; set; }

    [FirestoreProperty("location")]
    public string Location { get; set; } = "";

    [FirestoreProperty("capacity")]
    public int Capacity { get; set; }

    [FirestoreProperty("description")]
    public string Description { get; set; } = "";

    [FirestoreProperty("imageUrl")]
    public string ImageUrl { get; set; } = "";

    [FirestoreProperty("createdByUserId")]
    public string CreatedByUserId { get; set; } = "";

    [FirestoreProperty("createdByName")]
    public string CreatedByName { get; set; } = "";

    [FirestoreProperty("isPublic")]
    public bool IsPublic { get; set; } = true;

    [FirestoreProperty("createdAt")]
    public Timestamp CreatedAt { get; set; }

    [FirestoreProperty("participants")]
    public List<Participant> Participants { get; set; } = [];

    // Firestore array-contains sorguları için düz ID listesi
    [FirestoreProperty("participantIds")]
    public List<string> ParticipantIds { get; set; } = [];

    [FirestoreProperty("templateId")]
    public string TemplateId { get; set; } = "";
}
