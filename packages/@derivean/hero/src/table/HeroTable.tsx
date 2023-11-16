"use client";

import {HeroIcon}   from "@derivean/ui";
import {t}          from "@use-pico/i18n";
import {ButtonLink} from "@use-pico/ui";
import {
    type ComponentProps,
    type FC
}                   from "react";
import {HeroInline} from "../inline/HeroInline";
import {HeroUI}     from "../ui/HeroUI";

export namespace HeroTable {
    export type Columns =
        | "name"
        | "health"
        | "level"
        | "prestige";

    export type Props =
        Omit<
            ComponentProps<typeof HeroUI.Table<Columns>>,
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
    return <HeroUI.Table
        text={{
            total: t()`Total count of heroes`,
        }}
        name={"building"}
        icon={<HeroIcon/>}
        columns={{
            name:     {
                title:  t()`Hero name`,
                render: ({item}) => <ButtonLink
                    icon={<HeroIcon/>}
                    href={{
                        href:  "/kingdom/[kingdomId]/hero/[id]",
                        query: {
                            kingdomId,
                            id: item.id,
                        },
                    }}
                    label={<HeroInline entity={item}/>}
                />,
            },
            health:   {
                title:  t()`Hero health (label)`,
                render: ({item}) => "health progress bar",
                width:  28,
            },
            level:    {
                title:  t()`Hero level (label)`,
                render: ({item}) => item.level,
                width:  10,
            },
            prestige: {
                title:  t()`Hero prestige (label)`,
                render: ({item}) => "prestige component (colorful stars)",
                width:  10,
            },
        }}
        {...props}
    />;
};
