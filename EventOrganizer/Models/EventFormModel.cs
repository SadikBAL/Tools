using System.ComponentModel.DataAnnotations;

namespace EventOrganizer.Models;

public class EventFormModel
{
    [Required(ErrorMessage = "Başlık gerekli")]
    [MaxLength(100, ErrorMessage = "En fazla 100 karakter")]
    public string Title { get; set; } = "";

    [Required(ErrorMessage = "Kategori seçiniz")]
    public string Category { get; set; } = "music";

    [Required(ErrorMessage = "Tarih gerekli")]
    public DateTime Date { get; set; } = DateTime.Today.AddDays(7).AddHours(20);

    [Required(ErrorMessage = "Konum gerekli")]
    [MaxLength(200)]
    public string Location { get; set; } = "";

    [Range(1, 100000, ErrorMessage = "1 ile 100.000 arasında olmalı")]
    public int Capacity { get; set; } = 100;

    [MaxLength(500, ErrorMessage = "En fazla 500 karakter")]
    public string Description { get; set; } = "";

    public string ImageUrl { get; set; } = "";

    public bool IsPublic { get; set; } = true;
}
