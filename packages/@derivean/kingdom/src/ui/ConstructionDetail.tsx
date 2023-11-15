import {
    type BuildingSchema,
    withConstructionService
}                           from "@derivean/building";
import {
    ItemQueryStore,
    ItemTable
}                           from "@derivean/item";
import {container}          from "@derivean/server";
import {
    IconBarrierBlock,
    IconBarrierBlockOff
}                           from "@tabler/icons-react";
import {withDullSchema}     from "@use-pico/dull-stuff";
import {t}                  from "@use-pico/i18n";
import {StoreProvider}      from "@use-pico/store";
import {
    Alert,
    Box,
    Tabs,
    TabsList,
    TabsPanel,
    TabsTab
}                           from "@use-pico/ui";
import {type FC}            from "react";
import {type KingdomSchema} from "../schema/KingdomSchema";

export namespace ConstructionDetail {
    export interface Props {
        kingdom: withDullSchema.Infer.Entity<KingdomSchema>;
        building: withDullSchema.Infer.Entity<BuildingSchema>;
    }
}

export const ConstructionDetail: FC<ConstructionDetail.Props> = async (
    {
        kingdom,
        building,
    }
) => {
    const result = await withConstructionService.use(container).isAvailable(kingdom.inventoryId, building.id);
    return <>
        <Box
            mb={"xs"}
        >
            {result.result && <Alert
                icon={<IconBarrierBlock/>}
                color={"green.5"}
                title={t()`Construction is available`}
            >
                {t()`This building is available for construction.`}
            </Alert>}
            {!result.result && <Alert
                icon={<IconBarrierBlockOff/>}
                color={"red.5"}
                title={t()`Construction is not available`}
            >
                {t()`You do not have enough resources or workers to build this building. Check requirements.`}
            </Alert>}
        </Box>

        <h1>
            This is just a concept: create Item Table facing a player

            Or just render a simple list showing missing items + diff (of amount)
        </h1>

        <Tabs
            defaultValue={
                /**
                 * Do you know that kind of code you don't want to see? This is it.
                 *
                 * I'm sorry.
                 *
                 * This is problem of Mantine which **must have** default value for Tabs,
                 * so it won't pickup the first tab if there is not default.
                 *
                 * Piece of shit.
                 */
                result.missing.construction.length > 0 ?
                    "missing.construction"
                    : result.missing.required.length > 0 ?
                        "missing.required"
                        : result.notEnough.construction.length > 0 ?
                            "notEnough.construction"
                            : result.notEnough.required.length > 0 ?
                                "notEnough.required"
                                : "nope"
            }
        >
            <TabsList
                mb={"sm"}
            >
                {result.missing.construction.length > 0 && <TabsTab
                    value={"missing.construction"}
                >
                    {t()`Construction - missing items`}
                </TabsTab>}
                {result.missing.required.length > 0 && <TabsTab
                    value={"missing.required"}
                >
                    {t()`Missing required items`}
                </TabsTab>}
            </TabsList>

            {result.missing.construction.length > 0 && <TabsPanel
                value={"missing.construction"}
            >
                <StoreProvider
                    store={ItemQueryStore}
                    values={{
                        where: {
                            idIn: result.missing.construction,
                        },
                    }}
                >
                    <ItemTable/>
                </StoreProvider>
            </TabsPanel>}

            {result.missing.required.length > 0 && <TabsPanel
                value={"missing.required"}
            >
                <StoreProvider
                    store={ItemQueryStore}
                    values={{
                        where: {
                            idIn: result.missing.required,
                        },
                    }}
                >
                    <ItemTable
                        compact
                    />
                </StoreProvider>
            </TabsPanel>}
        </Tabs>
    </>;
};
