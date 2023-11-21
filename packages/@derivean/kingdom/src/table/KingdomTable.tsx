"use client";

import {KingdomIcon}       from "@derivean/ui";
import {ButtonLink}        from "@use-pico/client";
import {t}                 from "@use-pico/translator";
import {
    type ComponentProps,
    type FC
}                          from "react";
import {KingdomInline}     from "../inline/KingdomInline";
import {KingdomComponents} from "../ui/KingdomComponents";

export namespace KingdomTable {
    export type Columns =
        | "name";

    export type Props = Omit<
        ComponentProps<typeof KingdomComponents.Table<Columns>>,
        "columns" | "name" | "icon" | "text"
    >
}

export const KingdomTable: FC<KingdomTable.Props> = props => {
    return <KingdomComponents.Table
        text={{
            total: t()`Total count of kingdoms`,
            empty: {
                title:   t()`No kingdoms found`,
                message: t()`There are not kingdoms yet`,
            }
        }}
        icon={<KingdomIcon/>}
        // tableActionProps={{
        //     text:       {
        //         create: {
        //             title: t()`Create kingdom (modal)`,
        //             label: t()`Create kingdom`,
        //         },
        //     },
        //     upsertForm: ({modalId}) => <KingdomUpsertForm
        //         withAutoClose={[modalId]}
        //     />,
        // }}
        // rowActionProps={{
        //     text:         {
        //         update: {
        //             title: t()`Update kingdom`,
        //             label: t()`Update kingdom`,
        //         },
        //         delete: {
        //             label: t()`Delete kingdom`,
        //             modal: {
        //                 title:   t()`Delete kingdom (modal)`,
        //                 content: t()`Do you really want to delete selected kingdom?`,
        //                 success: {
        //                     title:   t()`Success`,
        //                     message: t()`Kingdom has been successfully removed`,
        //                 },
        //             },
        //         },
        //     },
        //     withMutation: KingdomRpc.mutation,
        //     upsertForm:   ({
        //                        item,
        //                        modalId
        //                    }) => <KingdomUpsertForm
        //         withAutoClose={[modalId]}
        //         entity={item}
        //     />,
        // }}
        columns={{
            name: {
                title:  t()`Kingdom name`,
                render: ({item}) => <ButtonLink
                    icon={<KingdomIcon/>}
                    href={{
                        href:  "/kingdom/[kingdomId]",
                        query: {
                            kingdomId: item.id,
                        },
                    }}
                >
                    <KingdomInline entity={item}/>
                </ButtonLink>,
            },
        }}
        {...props}
    />;
};
