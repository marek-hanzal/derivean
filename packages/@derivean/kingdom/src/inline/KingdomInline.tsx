import {withDullSchema}     from "@use-pico/dull-stuff";
import {td}                 from "@use-pico/i18n";
import {type WithEntity}    from "@use-pico/types";
import {type FC}            from "react";
import {type KingdomSchema} from "../schema/KingdomSchema";

export namespace KingdomInline {
    export interface Props extends WithEntity<withDullSchema.Infer.Entity<KingdomSchema>> {
    }
}

export const KingdomInline: FC<KingdomInline.Props> = (
    {
        entity,
    }
) => {
    return td()(`Kingdom [${entity.name}]`);
};
