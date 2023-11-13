import {
    BuildingPreview,
    withBuildingRepository
}                      from "@derivean/building";
import {BuildingMenu}  from "@derivean/manager";
import {ProducerGraph} from "@derivean/producer";
import {container}     from "@derivean/server";
import {BuildingIcon}  from "@derivean/ui";
import {t}             from "@use-pico/i18n";
import {
    Breadcrumbs,
    Grid,
    GridCol,
    HomeIcon,
    ListIcon,
    Page
}                      from "@use-pico/ui";

export namespace Index {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function Index({params: {id}}: Index.Props) {
    const building = await withBuildingRepository.use(container).withQuery.fetchOrThrow({
        where: {
            id,
        }
    });

    return <Page
        icon={<BuildingIcon/>}
        text={{
            header: t({values: building})`Building detail`,
        }}
        postfix={<Breadcrumbs
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  "/manager/building/list",
                    label: t()`Building list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
        append={<BuildingMenu
            buildingId={building.id}
            active={["/manager/building/[id]"]}
        />}
    >
        <Grid pb={"md"}>
            <GridCol span={6}>
                <BuildingPreview
                    building={building}
                />
            </GridCol>
            <GridCol span={6}>
                <ProducerGraph
                    producerId={building.producerId}
                />
            </GridCol>
        </Grid>
    </Page>;
}
