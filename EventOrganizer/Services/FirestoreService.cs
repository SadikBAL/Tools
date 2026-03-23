using EventOrganizer.Models;
using Google.Cloud.Firestore;
using Grpc.Core;

namespace EventOrganizer.Services;

public class FirestoreService
{
    private readonly FirestoreDb _db;

    public FirestoreService(FirestoreDb db)
    {
        _db = db;
    }

    // gRPC QUIC/HTTP3 handshake ilk bağlantıda zaman aşımına uğrayabilir;
    // bir kez yeniden dene — ikinci denemede HTTP/2 fallback devreye girer.
    private static async Task<T> WithRetry<T>(Func<Task<T>> fn)
    {
        try { return await fn(); }
        catch (RpcException ex) when (ex.StatusCode is StatusCode.Unavailable or StatusCode.DeadlineExceeded)
        {
            await Task.Delay(300);
            return await fn();
        }
    }

    private static async Task WithRetry(Func<Task> fn)
    {
        try { await fn(); }
        catch (RpcException ex) when (ex.StatusCode is StatusCode.Unavailable or StatusCode.DeadlineExceeded)
        {
            await Task.Delay(300);
            await fn();
        }
    }

    public Task<List<EventItem>> GetPublicEventsAsync() => WithRetry(async () =>
    {
        var snapshot = await _db.Collection("events")
            .WhereEqualTo("isPublic", true)
            .GetSnapshotAsync();

        return snapshot.Documents
            .Select(d => d.ConvertTo<EventItem>())
            .OrderBy(e => e.Date)
            .ToList();
    });

    public Task<List<EventItem>> GetUserEventsAsync(string userId) => WithRetry(async () =>
    {
        var snapshot = await _db.Collection("events")
            .WhereEqualTo("createdByUserId", userId)
            .GetSnapshotAsync();

        return snapshot.Documents
            .Select(d => d.ConvertTo<EventItem>())
            .OrderBy(e => e.Date)
            .ToList();
    });

    public Task<string> CreateEventAsync(EventItem evt) => WithRetry(async () =>
    {
        evt.CreatedAt = Timestamp.GetCurrentTimestamp();
        var docRef = await _db.Collection("events").AddAsync(evt);
        return docRef.Id;
    });

    public Task DeleteEventAsync(string id) => WithRetry(async () =>
        await _db.Collection("events").Document(id).DeleteAsync()
    );

    public Task<EventItem?> GetEventByIdAsync(string id) => WithRetry(async () =>
    {
        var snap = await _db.Collection("events").Document(id).GetSnapshotAsync();
        return snap.Exists ? snap.ConvertTo<EventItem>() : null;
    });

    // Kullanıcının oluşturduğu veya katıldığı tüm etkinlikler
    public Task<List<EventItem>> GetUserRelatedEventsAsync(string userId) => WithRetry(async () =>
    {
        var snapshot = await _db.Collection("events")
            .WhereArrayContains("participantIds", userId)
            .GetSnapshotAsync();

        return snapshot.Documents
            .Select(d => d.ConvertTo<EventItem>())
            .OrderBy(e => e.Date)
            .ToList();
    });

    public Task JoinEventAsync(string eventId, Participant participant) => WithRetry(async () =>
    {
        var docRef = _db.Collection("events").Document(eventId);
        await _db.RunTransactionAsync(async transaction =>
        {
            var snap = await transaction.GetSnapshotAsync(docRef);
            var evt  = snap.ConvertTo<EventItem>();
            if (!evt.ParticipantIds.Contains(participant.UserId) && evt.Participants.Count < evt.Capacity)
            {
                // Hedef slot doluysa iptal et
                if (participant.SlotIndex.HasValue &&
                    evt.Participants.Any(p => p.SlotIndex == participant.SlotIndex))
                    return;

                evt.Participants.Add(participant);
                evt.ParticipantIds.Add(participant.UserId);
                transaction.Update(docRef, new Dictionary<string, object>
                {
                    ["participants"]   = evt.Participants,
                    ["participantIds"] = evt.ParticipantIds,
                });
            }
        });
    });

    public Task MoveSlotAsync(string eventId, string userId, int newSlotIndex) => WithRetry(async () =>
    {
        var docRef = _db.Collection("events").Document(eventId);
        await _db.RunTransactionAsync(async transaction =>
        {
            var snap = await transaction.GetSnapshotAsync(docRef);
            var evt  = snap.ConvertTo<EventItem>();
            if (evt.Participants.Any(p => p.SlotIndex == newSlotIndex))
                return; // hedef slot dolu
            var p = evt.Participants.FirstOrDefault(p => p.UserId == userId);
            if (p == null) return;
            p.SlotIndex = newSlotIndex;
            transaction.Update(docRef, new Dictionary<string, object>
            {
                ["participants"] = evt.Participants,
            });
        });
    });

    public Task LeaveEventAsync(string eventId, string userId) => WithRetry(async () =>
    {
        var docRef = _db.Collection("events").Document(eventId);
        await _db.RunTransactionAsync(async transaction =>
        {
            var snap = await transaction.GetSnapshotAsync(docRef);
            var evt  = snap.ConvertTo<EventItem>();
            evt.Participants.RemoveAll(p => p.UserId == userId);
            evt.ParticipantIds.Remove(userId);
            transaction.Update(docRef, new Dictionary<string, object>
            {
                ["participants"]   = evt.Participants,
                ["participantIds"] = evt.ParticipantIds,
            });
        });
    });

    public Task UpdateEventAsync(EventItem evt) => WithRetry(async () =>
    {
        var updates = new Dictionary<string, object>
        {
            ["title"]       = evt.Title,
            ["category"]    = evt.Category,
            ["date"]        = Timestamp.FromDateTime(DateTime.SpecifyKind(evt.Date, DateTimeKind.Utc)),
            ["location"]    = evt.Location,
            ["capacity"]    = evt.Capacity,
            ["description"] = evt.Description,
            ["imageUrl"]    = evt.ImageUrl ?? "",
            ["isPublic"]    = evt.IsPublic,
        };
        await _db.Collection("events").Document(evt.Id).UpdateAsync(updates);
    });
}
