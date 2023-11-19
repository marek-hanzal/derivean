import {type WithEntity}    from "@use-pico/client";
import {type Infer}         from "@use-pico/extras";
import {type FC}            from "react";
import {type KingdomSchema} from "../schema/KingdomSchema";

export namespace KingdomInline {
    export interface Props extends WithEntity<Infer.Entity<KingdomSchema>> {
    }
}

export const KingdomInline: FC<KingdomInline.Props> = (
    {
        entity,
    }
) => {
    return entity.name;
};
