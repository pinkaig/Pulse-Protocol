using System;
using ScriptAPI;

public class OrderSlipCurrentIcon : Script
{
    private OrderSlipUIController? registeredWith = null;

    private float slotX, slotY;
    private string lastSprite = "";

    public override void Update()
    {
        var ui = OrderSlipUIController.Instance;
        if (ui == null) return;

        // Registers on first load AND re-registers on reload automatically. Cos json parent-child hieracy is not letting me change it in json
        if (registeredWith != ui)
        {
            TransformComponent tf0 = GetTransform();
            slotX = tf0.X;
            slotY = tf0.Y;
            ui.RegisterCurrentSlot(slotX, slotY); // or RegisterNextSlot for NextIcon
            registeredWith = ui;
            return;
        }

        TransformComponent tf = GetTransform();

        if (!ui.Transitioning)
        {
            tf.X = slotX;
            tf.Y = slotY;
            tf.ScaleX = ui.currentScale;
            tf.ScaleY = ui.currentScale;

            // Restore full opacity when idle
            SpriteComponent sprIdle = GetSprite();
            sprIdle.TintA = 1.0f;

            SetSprite(ui.GetVibrantSprite(ui.CurrentType));
            return;
        }

        // During transition: fade out via alpha (1 → 0 over fadeDuration)
        tf.X = slotX;
        tf.Y = slotY;
        tf.ScaleX = ui.currentScale;
        tf.ScaleY = ui.currentScale;

        SpriteComponent spr = GetSprite();
        spr.TintA = 1.0f - ui.Fade01;  // fades out as Fade01 goes 0 → 1

        SetSprite(ui.GetVibrantSprite(ui.CurrentType));
    }

    private void SetSprite(string spriteName)
    {
        if (string.IsNullOrEmpty(spriteName)) return; // RETURN IF NO ICON, COS WHY CHANGE 

        if (spriteName == lastSprite) return;
        lastSprite = spriteName;

        SpriteComponent spr = GetSprite();
        if (spr.HasSprite())
        {
            spr.Texture = spriteName;
            Console.WriteLine($"[OrderSlipCurrent] Sprite -> {spriteName}\n");
        }
        else
        {
            Console.WriteLine("[OrderSlipCurrent] WARNING: No Renderable/Sprite component on this entity");
        }
    }
}