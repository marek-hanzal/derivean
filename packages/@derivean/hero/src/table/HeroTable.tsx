"use client";

import {
    HeroIcon,
    Prestige
}                       from "@derivean/ui";
import {ButtonLink}     from "@use-pico/client";
import {t}              from "@use-pico/translator";
import {
    type ComponentProps,
    type FC
}                       from "react";
import {HeroInline}     from "../inline/HeroInline";
import {HeroComponents} from "../ui/HeroComponents";
import {HeroHealth}     from "../ui/HeroHealth";

export namespace HeroTable {
    export type Columns =
        | "name"
        | "health"
        | "level"
        | "prestige";

    export type Props =
        Omit<
            ComponentProps<typeof HeroComponents.Table<Columns>>,
            "columns" | "name" | "icon" | "text"
        >
        & {
            kingdomId: string;
        }
}

export const HeroTable: FC<HeroTable.Props> = (
    {
        kingdomId,
        ...props
    }
) => {
    return <HeroComponents.Table
        text={{
            total: t`Total count of heroes`,
        }}
        icon={<HeroIcon/>}
        columns={{
            name:     {
                title:  t`Hero name`,
                render: ({item}) => <ButtonLink
                    icon={<HeroIcon/>}
                    href={{
                        href:  "/kingdom/[kingdomId]/hero/[id]",
                        query: {
                            kingdomId,
                            id: item.id,
                        },
                    }}
                >
                    <HeroInline entity={item}/>
                </ButtonLink>,
            },
            health:   {
                title:  t`Hero health (label)`,
                render: ({item}) => <HeroHealth entity={item}/>,
                width:  "w-54",
            },
            level:    {
                title: t`Hero level (label)`,
                render: ({item}) => item.level,
                width: "w-54",
            },
            prestige: {
                title: t`Hero prestige (label)`,
                render: ({item}) => <Prestige prestige={item.prestige}/>,
                width: "w-54",
            },
        }}
        {...props}
    />;
};
