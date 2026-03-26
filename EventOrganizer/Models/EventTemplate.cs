using Google.Cloud.Firestore;

namespace EventOrganizer.Models;

[FirestoreData]
public class EventTemplate
{
    [FirestoreDocumentId]
    public string Id { get; set; } = "";

    [FirestoreProperty("name")]
    public string Name { get; set; } = "";

    [FirestoreProperty("category")]
    public string Category { get; set; } = "";

    [FirestoreProperty("capacity")]
    public int Capacity { get; set; }

    [FirestoreProperty("rows")]
    public int Rows { get; set; }

    [FirestoreProperty("cols")]
    public int Cols { get; set; }

    [FirestoreProperty("activeSlots")]
    public List<TemplateSlot> ActiveSlots { get; set; } = [];

    [FirestoreProperty("svgUrl")]
    public string SvgUrl { get; set; } = "";

    [FirestoreProperty("createdByUserId")]
    public string CreatedByUserId { get; set; } = "";

    [FirestoreProperty("createdAt")]
    public Timestamp CreatedAt { get; set; }
}
