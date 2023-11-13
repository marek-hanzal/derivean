"use client";

import {withDullSchema} from "@use-pico/dull-stuff";
import {t}              from "@use-pico/i18n";
import {
    Group,
    Preview,
    Text
}                       from "@use-pico/ui";
import {type FC}        from "react";
import {KingdomInline}  from "../inline/KingdomInline";
import {KingdomSchema}  from "../schema/KingdomSchema";

export namespace KingdomPreview {
    export interface Props {
        kingdom: withDullSchema.Infer.Entity<KingdomSchema>;
    }
}

export const KingdomPreview: FC<KingdomPreview.Props> = (
    {
        kingdom,
    }
) => {
    return <Preview
        cols={3}
        items={[
            {
                label: t()`Kingdom name`,
                value: <Group gap={"xs"}>
                           <KingdomInline entity={kingdom}/>
                           <Text c={"dimmed"}>
                               ({kingdom.name})
                           </Text>
                       </Group>,
            },
        ]}
    />;
};
